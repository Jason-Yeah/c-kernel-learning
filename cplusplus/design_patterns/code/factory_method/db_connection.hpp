#pragma once

#include <iostream>
#include <string>

class DBConnection
{
public:
    virtual ~DBConnection() = default;
    virtual void connect(const std::string &dsn) = 0;
    virtual void query(const std::string &sql) = 0;
};

// should spliting to other `.hpp` files
class MySQLConnection : public DBConnection
{
public:
    void connect(const std::string &dsn) override
    {
        std::cout << "[MYSQL] is connected: " << dsn << std::endl;
    }

    void query(const std::string &sql) override
    {
        std::cout << "[MYSQL] executes: " << sql << std::endl;
    }
};

class PostgreSQLConnection : public DBConnection
{
public:
    void connect(const std::string &dsn) override
    {
        std::cout << "[POSTGRESQL] is connected: " << dsn << std::endl;
    }

    void query(const std::string &sql) override
    {
        std::cout << "[POSTGRESQL] executes: " << sql << std::endl;
    }
};
