
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
#ifndef KERNELv4
#	include <linux/cdev.h>
#endif

#include "box_ipc.h"

#define BOXIPC_MAX_MEM		(4 * 1024 * 1024)
#define BOXIPC_MNAME 		"box_ipc"
#define BOXIPC_LOGID 		BOXIPC_MNAME ": "
#define BOXIPC_PROC_ENTRY 	BOXIPC_MNAME


typedef struct _taskdata_t
{
	unsigned long id;
	long mcount;
	wait_queue_head_t wq;
	struct list_head task_list;
	struct list_head msg_list;
} taskdata_t;

typedef struct _msgitem_t
{
	unsigned long len;
	struct list_head msg_list;
	struct box_ipc_msg_t bim;
} msgitem_t;

#define SIZEOF_MI(_len) (sizeof(msgitem_t) - sizeof(struct box_ipc_msg_t) + _len)

static long mem_count = 0;
static LIST_HEAD(task_list);
#ifdef DECLARE_MUTEX
static DECLARE_MUTEX(boxipc_mtx);
#else
static DEFINE_SEMAPHORE(boxipc_mtx);
#endif

static struct proc_dir_entry *boxipc_proc_entry = NULL;

static dev_t devbase = MKDEV(205, 0);
#ifndef KERNELv4
struct cdev boxipc_cdev;
#endif


/*  /dev/box_ipc file operations */

static int boxipc_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int boxipc_release(struct inode *inode, struct file *file)
{
	down(&boxipc_mtx);

	if(file->private_data)
	{
		msgitem_t *mi, *n;
		taskdata_t *td = (taskdata_t *)file->private_data;
		list_del(&td->task_list);
		list_for_each_entry_safe(mi, n, &td->msg_list, msg_list)
		{
			list_del(&mi->msg_list);
			mem_count -= SIZEOF_MI(mi->len);
			kfree(mi);
		}
		mem_count -= sizeof(taskdata_t);
		kfree(td);
		file->private_data = NULL;
//		printk(KERN_INFO BOXIPC_LOGID "Task released (%08lx)\n", td->id);
	}
	
	up(&boxipc_mtx);
	return 0;
}

static ssize_t boxipc_read(struct file *file, char *buf, size_t len, loff_t *lff) 
{
	ssize_t ret = 0;
	if(down_interruptible(&boxipc_mtx))
	{
		return -ERESTARTSYS;
	}

	if(file->private_data == NULL)
	{
		ret = -ENODEV;
	}
	else
	{
		taskdata_t *td = (taskdata_t *)file->private_data;
		if(!(file->f_flags & O_NONBLOCK))
		{
			while(td->mcount <= 0)
			{
				up(&boxipc_mtx);
				if(wait_event_interruptible(td->wq, (td->mcount > 0)))
				{
					return -ERESTARTSYS;
				}
				if(down_interruptible(&boxipc_mtx))
				{
					return -ERESTARTSYS;
				}
			}
		}
		if(!list_empty(&td->msg_list))
		{
			msgitem_t *mi = list_entry(td->msg_list.next, msgitem_t, msg_list);
			ret = mi->len;
			if(len >= ret)
			{
				if(copy_to_user(buf, &mi->bim, ret))
				{
					ret = -EFAULT;
				}
				else
				{
					td->mcount--;
					list_del(&mi->msg_list);
					mem_count -= SIZEOF_MI(mi->len);
					kfree(mi);
				}
			}			
		}
	}
	
	up(&boxipc_mtx);
	return ret;
}

static ssize_t boxipc_write(struct file *file, const char *buf, size_t len, loff_t *lff)
{
	ssize_t ret = 0;
	struct box_ipc_msg_t bim;
	if(len < sizeof(bim))
	{
		return -EINVAL;
	}
	if (copy_from_user(&bim, buf, sizeof(bim)))
	{
		return -EFAULT;
	}
	if(down_interruptible(&boxipc_mtx))
	{
		return -ERESTARTSYS;
	}
	if(file->private_data == NULL)
	{
		if((len == sizeof(bim)) && (bim.type == BOXIPC_MSG_TYPE_REGISTER))
		{
			taskdata_t *td;
			list_for_each_entry(td, &task_list, task_list)
			{
				if(td->id == bim.from)
				{
					up(&boxipc_mtx);
					return -EBUSY;
				}
			}
			td = kmalloc(sizeof(taskdata_t), GFP_KERNEL);
			if(!td)
			{
				ret = -ENOMEM;
			}
			else
			{
				mem_count += sizeof(taskdata_t);
				td->id = bim.from;
				td->mcount = 0;
				init_waitqueue_head(&td->wq);
				INIT_LIST_HEAD(&td->task_list);
				INIT_LIST_HEAD(&td->msg_list);
				list_add_tail(&td->task_list, &task_list);
				file->private_data = td;
//				printk(KERN_INFO BOXIPC_LOGID "Task registered (%08lx)\n", bim.from);
				ret = len;
			}
		}
		else
		{
			ret = -EINVAL;
		}
	}
	else
	{
		taskdata_t *tdd, *tdo = (taskdata_t *)file->private_data;
		list_for_each_entry(tdd, &task_list, task_list)
		{
			if(tdd->id == bim.from || (bim.from == BOXIPC_ID_BROADCAST && tdo != tdd) 
				|| (!(bim.from & (~BOXIPC_GROUP_MASK)) && ((tdd->id & BOXIPC_GROUP_MASK) == bim.from)))
			{
				if(mem_count + len > BOXIPC_MAX_MEM)
				{
					ret = -ENOMEM;
				}
				else
				{
					msgitem_t *mi = kmalloc(SIZEOF_MI(len), GFP_KERNEL);
					if(!mi)
					{
						ret = -ENOMEM;
					}
					else
					{
						if(copy_from_user(&mi->bim, buf, len))
						{
							kfree(mi);
							ret = -EFAULT;
						}
						else
						{
							mi->len = len;
							INIT_LIST_HEAD(&mi->msg_list);
							mi->bim.from = tdo->id;
							mi->bim.type = bim.type;
							mem_count += SIZEOF_MI(len);
							list_add_tail(&mi->msg_list, &tdd->msg_list);
							tdd->mcount++;
							ret = len;
							wake_up_interruptible(&tdd->wq);
						}
					}
				}
				if(tdd->id == bim.from || ret < 0) break;
			}
		}
	}
	
	up(&boxipc_mtx);
	return ret;
}

static unsigned int boxipc_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	down(&boxipc_mtx);
	if(file->private_data != NULL)
	{
		taskdata_t *td = (taskdata_t *)file->private_data;
		poll_wait(file, &td->wq, wait);
		if(td->mcount > 0)
		{
			mask |= POLLIN | POLLRDNORM;
		}
	}
	up(&boxipc_mtx);
	return mask;
}

static struct file_operations boxipc_fops = {
	.owner = THIS_MODULE,
	.open  = boxipc_open,
	.release = boxipc_release,
	.read  = boxipc_read,
	.write = boxipc_write,
	.poll = boxipc_poll,
};

/*  /proc/box_ipc file operations */

static ssize_t boxipc_proc_read(struct file *file, char *buf, size_t len, loff_t *lff)
{
	ssize_t ret = 0, n = 0, c = 0;
	char buffer[128];
	taskdata_t *td;

	if(file->private_data)
	{
		file->private_data = (void*)0;
		return 0;
	}
	file->private_data = (void*)1;

	if(down_interruptible(&boxipc_mtx))
	{
		return -ERESTARTSYS;
	}

	list_for_each_entry(td, &task_list, task_list)
	{
		c += td->mcount;
		n = snprintf(buffer, sizeof(buffer), "Task %08lx: %ld messages\n", td->id, td->mcount);
		if((len < (ret + n)) || copy_to_user(buf + ret, buffer, n))
		{
			up(&boxipc_mtx);
			return -EFAULT;
		}
		ret += n;
	}
	n = snprintf(buffer, sizeof(buffer), "\nTotal: %d messages\nMemory in use: %ld Bytes\n", c, mem_count);
	if((len < (ret + n)) || copy_to_user(buf + ret, buffer, n))
	{
		up(&boxipc_mtx);
		return -EFAULT;
	}
	ret += n;

	up(&boxipc_mtx);
	return ret;
}

static struct file_operations boxipc_proc_ops = {
	.owner = THIS_MODULE,
	.read = boxipc_proc_read,
};

static int __init boxipc_init(void)
{
	int result;
#ifndef KERNELv4
	if((result = register_chrdev_region(devbase, 1, BOXIPC_MNAME)) < 0)
	{
		printk(KERN_WARNING BOXIPC_LOGID "register_chrdev_region %d\n", MAJOR(devbase));
		return result;
	}
	cdev_init(&boxipc_cdev, &boxipc_fops);
	boxipc_cdev.owner = THIS_MODULE;
	boxipc_cdev.ops = &boxipc_fops;
	if((result = cdev_add(&boxipc_cdev, devbase, 1)))
	{
		printk(KERN_WARNING BOXIPC_LOGID "cdev_add %d:%d\n", MAJOR(devbase), MINOR(devbase));
		return result;
	}
#else
	if((result = register_chrdev(MAJOR(devbase), BOXIPC_MNAME, &boxipc_fops)) < 0)
	{
		printk(KERN_WARNING BOXIPC_LOGID "register_chrdev %d\n", MAJOR(devbase));
		return result;
	}
#endif
#ifndef DECLARE_MUTEX //kernel v3.13
	boxipc_proc_entry = proc_create(BOXIPC_PROC_ENTRY, S_IFREG | 0666, NULL, &boxipc_proc_ops);
	if (!boxipc_proc_entry)
		printk(KERN_WARNING BOXIPC_LOGID "error creating proc entry '" BOXIPC_PROC_ENTRY "'\n");
#else
	boxipc_proc_entry = create_proc_entry(BOXIPC_PROC_ENTRY, S_IFREG | 0666, NULL);
	if(!boxipc_proc_entry)
	{
		printk(KERN_WARNING BOXIPC_LOGID "error creating proc entry '" BOXIPC_PROC_ENTRY "'\n");
	}
	else
	{
		boxipc_proc_entry->proc_fops = &boxipc_proc_ops;
	}
#endif
	printk(KERN_INFO BOXIPC_LOGID "Started OK (v0.1.2 - " __DATE__ ")\n");
	return 0;
}

static void __exit boxipc_exit(void)
{
	printk(KERN_INFO BOXIPC_LOGID "Exiting ...\n");
	if(boxipc_proc_entry) remove_proc_entry(BOXIPC_PROC_ENTRY, NULL);
#ifndef KERNELv4
	cdev_del(&boxipc_cdev);
	unregister_chrdev_region(devbase, 1);
#else
	if(unregister_chrdev(MAJOR(devbase), BOXIPC_MNAME) < 0)
		printk(KERN_WARNING BOXIPC_LOGID "unregister_chrdev %d\n", MAJOR(devbase));
#endif
}

module_init(boxipc_init);
module_exit(boxipc_exit);
MODULE_LICENSE("Dual BSD/GPL");

