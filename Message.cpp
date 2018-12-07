
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "utils.h"
#include "Message.h"

NodeElement::NodeElement(const NodeElement &e)
 : values(NULL), nodes(NULL), dup(true)
{
	tag = strdup(e.tag);
}

NodeElement::~NodeElement()
{
	if (dup) {
		free(const_cast<char*>(tag));
		delete values;
		delete nodes;
	}
}

MsgNode::MsgNode()
{
	copies = new int(1);
	elements = new std::set<NodeElement>;
}

MsgNode::MsgNode(void *xml_node)
{
	copies = new int(1);
	elements = new std::set<NodeElement>;
	create(xml_node);
}

MsgNode::~MsgNode()
{
	if((*copies)-- == 1)
	{
		delete elements;
		delete copies;
	}
}

MsgNode::MsgNode(const MsgNode &mnode)
{
	(*mnode.copies)++;
	copies = mnode.copies;
	elements = mnode.elements;
}

void MsgNode::create(void *xml_node)
{
	xmlNodePtr node = ((xmlNodePtr)xml_node)->children;
	while(node)
	{
		if(node->type == XML_ELEMENT_NODE && node->name && node->children)
		{
			if(node->children->content && (node->children->type == XML_TEXT_NODE
						|| node->children->type == XML_CDATA_SECTION_NODE))
			{
				const char *value = (const char *)node->children->content;
				while(isspace(*value)) value++;
				if(*value)
				{
//fprintf(stderr, "Adding value: %s : %s\n", (const char *)node->name, value);
					addValue((const char *)node->name, value);
					node = node->next;
					continue;
				}
			}
//fprintf(stderr, "Adding node: %s\n", (const char *)node->name);
			addNode((const char *)node->name, MsgNode(node));
		}
		node = node->next;
	}
}

void MsgNode::serialize(std::string *sbuf) const
{
	for(std::set<NodeElement>::const_iterator it = elements->begin(); it != elements->end(); it++)
	{
		if (it->values) for(int i = 0; i < (int)it->values->size(); i++) {
			sbuf->push_back('<'); sbuf->append(it->tag); sbuf->push_back('>');
			sbuf->append(it->values->at(i).c_str(), it->values->at(i).length());
			sbuf->push_back('<'); sbuf->push_back('/'); sbuf->append(it->tag); sbuf->push_back('>');
		}
		if (it->nodes) for(int i = 0; i < (int)it->nodes->size(); i++) {
			sbuf->push_back('<'); sbuf->append(it->tag); sbuf->push_back('>');
			it->nodes->at(i).serialize(sbuf);
			sbuf->push_back('<'); sbuf->push_back('/'); sbuf->append(it->tag); sbuf->push_back('>');
		}
	}
}	

const MsgNode *MsgNode::getNode(const char *name, int j) const
{
	std::set<NodeElement>::iterator it = elements->find(name);
	if (it != elements->end() && it->nodes && (int)it->nodes->size() > j) {
		return it->nodes->at(j).mthis();
	}
	return NULL;
}

const char *MsgNode::getValue(const char *name, int j) const
{
	std::set<NodeElement>::iterator it = elements->find(name);
	if (it != elements->end() && it->values && (int)it->values->size() > j) {
		return it->values->at(j).c_str();
	}
	return "";
}

int MsgNode::getInt(const char *name, int j) const
{
	return atoi(getValue(name, j));
}

bool MsgNode::getBool(const char *name, int j) const
{
	return utils::xmlToBool(getValue(name, j));
}

void MsgNode::addNode(const char *name, const MsgNode &node)
{
	NodeElement ne = name;
	std::set<NodeElement>::iterator it = elements->find(ne);
	if(it == elements->end())
	{
		it = elements->insert(elements->begin(), ne);
	}
	if(!it->nodes) it->nodes = new std::vector<MsgNode>;
	it->nodes->push_back(node);
}

void MsgNode::addValue(const char *name, const char *value)
{
	NodeElement ne = name;
	std::set<NodeElement>::iterator it = elements->find(ne);
	if(it == elements->end())
	{
		it = elements->insert(elements->begin(), ne);
	}
	if(!it->values) it->values = new std::vector<std::string>;
	it->values->push_back(value);
}
void MsgNode::addValue(const char *name, const std::string &value)
{
	addValue(name, value.c_str());
}
void MsgNode::addValue(const char *name, int value)
{
	char buf[32];
	addValue(name, utils::toString(buf, value));
}
void MsgNode::addValue(const char *name, bool value)
{
	addValue(name, value?"1":"0");
}

Message *Message::create(const char *msg, int len)
{
	Message *message = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;

	if ((doc = xmlParseMemory(msg, len ? : strlen(msg))))
	{
		node = xmlDocGetRootElement(doc);
		while(node)
		{			
			if(node->type == XML_ELEMENT_NODE && !strcasecmp("msg", (const char *)node->name))
			{
				node = node->children;
				continue;
			}
			if(node->type == XML_ELEMENT_NODE && ((const char *)node->name)[0] == 'M')
			{
				int id = atoi((const char *)node->name + 1);
				if(id > 0)
				{
					message = new Message(id);
					message->body.create(node);
					break;
				}
			}
			node = node->next;
		}
		xmlFreeDoc(doc);
	}
	return message;
}

void Message::serialize(std::string *sbuf) const
{
	char buf[16];
	int l = sprintf(buf, "%d", id);
	*sbuf = "<msg>";
	*sbuf += "<M"; sbuf->append(buf, l); *sbuf += ">";
	body.serialize(sbuf);
	*sbuf += "</M"; sbuf->append(buf, l); *sbuf += "></msg>";
}

int Message::getId(const char *msg, int len)
{
	for (int i = 5; i < (len - 10); i++) {
		if (msg[i] == '<' && msg[i+1] == 'M') {
			return atoi(msg + i + 2);
		}
	}
	return 0;
}
