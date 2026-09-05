#include <iostream>
#include <vector>

class AuditRecord
{
public:
    explicit AuditRecord(int requestId) : requestId_(requestId)
    {
        std::cout << "construct record for " << requestId_ << '\n';
    }
    ~AuditRecord() { std::cout << "destroy record for " << requestId_ << '\n'; }

private:
    int requestId_{};
};

bool isValid(int requestId) { return requestId >= 0; }

void process(const std::vector<int> &requests)
{
    for (int requestId : requests)
    {
        if (!isValid(requestId))
        {
            std::cout << "skip invalid request\n";
            continue;
        }

        AuditRecord record{requestId}; // 仅在真正需要时创建
        std::cout << "handle request " << requestId << '\n';
    } // record 的作用域精确覆盖一次有效请求处理
}

int main() { process({-1, 42}); }
