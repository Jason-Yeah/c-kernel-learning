#pragma once

class File;
class Directory;

class NodeVisitor
{
public:
    virtual ~NodeVisitor() = default;
    virtual void visit(const File &) = 0;
    virtual void visit(const Directory &) = 0;
};
