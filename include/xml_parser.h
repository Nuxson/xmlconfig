#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace tinyxml2
{
    class XMLElement;
    class XMLDocument;
}

// Изменили название чтобы избежать конфликта
struct MyXMLNode
{
    std::string name;
    std::string value;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<std::unique_ptr<MyXMLNode>> children;
    MyXMLNode *parent = nullptr;

    MyXMLNode *findChild(const std::string &childName);
    std::string getAttribute(const std::string &attrName, const std::string &defaultValue = "");
    MyXMLNode *addChild(const std::string &childName);
    void print(int indent = 0) const;
};

class XMLParser
{
public:
    XMLParser();
    ~XMLParser();

    bool loadFromFile(const std::string &filename);
    bool loadFromString(const std::string &xmlContent);
    bool saveToFile(const std::string &filename);
    MyXMLNode *getRoot() { return root_.get(); }
    void createNewDocument(const std::string &rootName);
    MyXMLNode *findNodeByPath(const std::string &path);

private:
    std::unique_ptr<MyXMLNode> root_;
    std::unique_ptr<tinyxml2::XMLDocument> document_;

    std::unique_ptr<MyXMLNode> parseElement(tinyxml2::XMLElement *element);
    void saveElement(tinyxml2::XMLElement *parentElem, MyXMLNode *node);
};