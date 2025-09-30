#include "../include/xml_parser.h"
#include "../third_party/tinyxml2/tinyxml2.h"
#include <iostream>
#include <sstream>

// Указываем использование только наших классов
using namespace tinyxml2;

MyXMLNode *MyXMLNode::findChild(const std::string &childName)
{
    for (auto &child : children)
    {
        if (child->name == childName)
            return child.get();
    }
    return nullptr;
}

std::string MyXMLNode::getAttribute(const std::string &attrName, const std::string &defaultValue)
{
    auto it = attributes.find(attrName);
    return it != attributes.end() ? it->second : defaultValue;
}

MyXMLNode *MyXMLNode::addChild(const std::string &childName)
{
    auto newChild = std::make_unique<MyXMLNode>();
    newChild->name = childName;
    newChild->parent = this;
    auto *ptr = newChild.get();
    children.push_back(std::move(newChild));
    return ptr;
}

void MyXMLNode::print(int indent) const
{
    std::string indentStr(indent * 2, ' ');
    std::cout << indentStr << "<" << name;

    for (const auto &attr : attributes)
    {
        std::cout << " " << attr.first << "=\"" << attr.second << "\"";
    }

    if (value.empty() && children.empty())
    {
        std::cout << "/>" << std::endl;
    }
    else
    {
        std::cout << ">";

        if (!value.empty())
        {
            std::cout << value;
        }

        if (!children.empty())
        {
            std::cout << std::endl;
            for (const auto &child : children)
            {
                child->print(indent + 1);
            }
            std::cout << indentStr;
        }

        std::cout << "</" << name << ">" << std::endl;
    }
}

XMLParser::XMLParser() = default;
XMLParser::~XMLParser() = default;

bool XMLParser::loadFromFile(const std::string &filename)
{
    document_ = std::make_unique<XMLDocument>();

    if (document_->LoadFile(filename.c_str()) != XML_SUCCESS)
    {
        std::cout << "Failed to load file: " << filename << std::endl;
        return false;
    }

    XMLElement *rootElem = document_->RootElement();
    if (!rootElem)
    {
        std::cout << "No root element found" << std::endl;
        return false;
    }

    root_ = parseElement(rootElem);
    if (root_)
    {
        std::cout << "Successfully loaded: " << filename << std::endl;
    }
    return root_ != nullptr;
}

bool XMLParser::loadFromString(const std::string &xmlContent)
{
    document_ = std::make_unique<XMLDocument>();

    if (document_->Parse(xmlContent.c_str(), xmlContent.size()) != XML_SUCCESS)
    {
        return false;
    }

    XMLElement *rootElem = document_->RootElement();
    if (!rootElem)
    {
        return false;
    }

    root_ = parseElement(rootElem);
    return root_ != nullptr;
}

bool XMLParser::saveToFile(const std::string &filename)
{
    if (!document_)
    {
        document_ = std::make_unique<XMLDocument>();
    }

    document_->Clear();

    // Добавляем декларацию XML
    auto decl = document_->NewDeclaration();
    document_->InsertFirstChild(decl);

    if (root_)
    {
        auto rootElem = document_->NewElement(root_->name.c_str());
        document_->InsertEndChild(rootElem);
        saveElement(rootElem, root_.get());
    }

    bool success = document_->SaveFile(filename.c_str()) == XML_SUCCESS;
    if (success)
    {
        std::cout << "Successfully saved: " << filename << std::endl;
    }
    else
    {
        std::cout << "Failed to save: " << filename << std::endl;
    }
    return success;
}

void XMLParser::createNewDocument(const std::string &rootName)
{
    root_ = std::make_unique<MyXMLNode>();
    root_->name = rootName;
    document_ = std::make_unique<XMLDocument>();
}

MyXMLNode *XMLParser::findNodeByPath(const std::string &path)
{
    if (!root_)
        return nullptr;

    std::istringstream pathStream(path);
    std::string segment;
    MyXMLNode *current = root_.get();

    while (std::getline(pathStream, segment, '/'))
    {
        if (segment.empty())
            continue;
        current = current->findChild(segment);
        if (!current)
            return nullptr;
    }

    return current;
}

std::unique_ptr<MyXMLNode> XMLParser::parseElement(tinyxml2::XMLElement *element)
{
    if (!element)
        return nullptr;

    auto node = std::make_unique<MyXMLNode>();
    node->name = element->Name();

    // Парсим атрибуты
    for (const XMLAttribute *attr = element->FirstAttribute(); attr; attr = attr->Next())
    {
        node->attributes[attr->Name()] = attr->Value();
    }

    // Парсим текстовое содержимое
    const char *text = element->GetText();
    if (text)
    {
        node->value = text;
    }

    // Рекурсивно парсим дочерние элементы
    for (XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        auto childNode = parseElement(child);
        if (childNode)
        {
            childNode->parent = node.get();
            node->children.push_back(std::move(childNode));
        }
    }

    return node;
}

void XMLParser::saveElement(tinyxml2::XMLElement *parentElem, MyXMLNode *node)
{
    if (!node || !parentElem)
        return;

    // Устанавливаем атрибуты
    for (const auto &attr : node->attributes)
    {
        parentElem->SetAttribute(attr.first.c_str(), attr.second.c_str());
    }

    // Устанавливаем текстовое значение
    if (!node->value.empty())
    {
        parentElem->SetText(node->value.c_str());
    }

    // Сохраняем дочерние элементы
    for (auto &child : node->children)
    {
        auto childElem = document_->NewElement(child->name.c_str());
        parentElem->InsertEndChild(childElem);
        saveElement(childElem, child.get());
    }
}