#pragma once

#include "db_connection.hpp"
#include <iostream>
#include <memory>
#include <string>

class DBFactory
{
public:
    virtual ~DBFactory() = default;

    virtual std::unique_ptr<DBConnection> createConn() = 0;

    void runQuery(const std::string &dsn, const std::string &sql)
    {
        auto conn = createConn();
        conn->connect(dsn);
        conn->query(sql);
    }
};
