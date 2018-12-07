
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdlib.h>
#include <string.h>
#include <set>
#include <vector>
#include <string>

class MsgNode;

class NodeElement
{
	friend class MsgNode;	
	const char *tag;
	mutable std::vector<std::string> *values;
	mutable std::vector<MsgNode> *nodes;
	bool dup;
	const NodeElement *mthis() const { return this; };
	NodeElement(const char *t) : tag(t), values(NULL), nodes(NULL), dup(false) {};
public:
	NodeElement(const NodeElement &e);
	~NodeElement();
	bool operator<(const NodeElement &e) const { return (strcmp(tag, e.tag) < 0); };
};

class Message;

class MsgNode
{
	friend class Message;
	int *copies;
	std::set<NodeElement> *elements;
#ifdef RETRO_COMPATIBILITY
	std::vector<const NodeElement*> *fifo;
#endif
	void create(void *body);
	void serialize(std::string *buf) const;
	MsgNode(void *);
public:
	MsgNode();
	~MsgNode();
	MsgNode(const MsgNode &mnode);
	const MsgNode *mthis() const { return this; };
	const MsgNode *getNode(const char *name, int j = 0) const;
	const char *getValue(const char *name, int j = 0) const;
	int getInt(const char *name, int j = 0) const;
	bool getBool(const char *name, int j = 0) const;
	void addNode(const char *name, const MsgNode &node);
	void addValue(const char *name, const std::string &value);
	void addValue(const char *name, const char *value);
	void addValue(const char *name, int value);
	void addValue(const char *name, bool value);
};

class Message
{
	int id;
	MsgNode body;
	Message();
public:
	Message(int id) : id(id) {};
	static Message *create(const char *msg, int len = 0);
	void serialize(std::string *buf) const;
	int getId() const { return id; };
	const MsgNode *getNode(const char *name, int j = 0)	const { return body.getNode(name, j); };
	const char *getValue(const char *name, int j = 0) const { return body.getValue(name, j); };
	int getInt(const char *name, int j = 0) const { return body.getInt(name, j); };
	bool getBool(const char *name, int j = 0) const { return body.getBool(name, j); };
	void addNode(const char *name, const MsgNode &node) { body.addNode(name, node); };
	void addValue(const char *name, const std::string &value) { body.addValue(name, value); };
	void addValue(const char *name, const char *value) { body.addValue(name, value); };
	void addValue(const char *name, int value) { body.addValue(name, value); };
	void addValue(const char *name, bool value) { body.addValue(name, value); };
	static int getId(const char *msg, int len);
};

#endif /*MESSAGE_H_*/
