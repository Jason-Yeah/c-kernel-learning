#include <iostream>
#include <ucontext.h>
#include <vector>

// 1. 定义协程的结构体（上下文和独立的栈空间）
struct Coroutine {
    ucontext_t ctx;          // 保存 CPU 寄存器状态的结构体
    char* stack;             // 协程独立的私有栈空间
    bool is_finished;        // 协程是否运行结束
    
    Coroutine() : stack(new char[1024 * 64]), is_finished(false) {} // 每个协程分配 64KB 的小栈
    ~Coroutine() { delete[] stack; }
};

// 全局全局调度器变量
std::vector<Coroutine*> schedule;
ucontext_t main_ctx;         // 线程主控制流的上下文（代表调度器自己）
int current_id = -1;         // 当前正在运行的协程 ID

// 2. 核心操作：Yield（协程主动让出 CPU）
void coroutine_yield() {
    if (current_id == -1) return;
    
    int last_id = current_id;
    current_id = -1; // 切回主控制流
    
    std::cout << "   [协程 " << last_id << "] 遇到耗时操作，执行 yield 挂起，保存当前寄存器...\n";
    
    // 💡 核心对调：保存当前协程的寄存器到 schedule[last_id]->ctx，并恢复主线程 main_ctx 的寄存器
    swapcontext(&(schedule[last_id]->ctx), &main_ctx);
}

// 3. 协程的具体工作函数
void func_A() {
    std::cout << ">> [协程 0] 开始运行...\n";
    std::cout << ">> [协程 0] 正在处理第一阶段任务。\n";
    
    coroutine_yield(); // 👈 主动让出 CPU！
    
    std::cout << ">> [协程 0] 满血复活！继续处理第二阶段任务。\n";
    std::cout << ">> [协程 0] 运行结束。\n";
    schedule[0]->is_finished = true;
}

void func_B() {
    std::cout << ">> [协程 1] 开始运行...\n";
    std::cout << ">> [协程 1] 正在处理第一阶段任务。\n";
    
    coroutine_yield(); // 👈 主动让出 CPU！
    
    std::cout << ">> [协程 1] 满血复活！继续处理第二阶段任务。\n";
    std::cout << ">> [协程 1] 运行结束。\n";
    schedule[1]->is_finished = true;
}

int main() {
    std::cout << "[主线程] 正在初始化协程 A 和 B...\n";
    Coroutine co0, co1;
    schedule.push_back(&co0);
    schedule.push_back(&co1);

    // 4. 为协程配置独立的上下文和私有栈（这一步完全在用户态，由我们自己手动分配）
    getcontext(&co0.ctx);
    co0.ctx.uc_stack.ss_sp = co0.stack;
    co0.ctx.uc_stack.ss_size = 1024 * 64;
    co0.ctx.uc_link = &main_ctx; // 协程结束后自动回到主上下文
    makecontext(&co0.ctx, (void(*)())func_A, 0); // 绑定函数

    getcontext(&co1.ctx);
    co1.ctx.uc_stack.ss_sp = co1.stack;
    co1.ctx.uc_stack.ss_size = 1024 * 64;
    co1.ctx.uc_link = &main_ctx;
    makecontext(&co1.ctx, (void(*)())func_B, 0);

    // 5. 简单的调度器循环：只要协程没结束，就轮流 Resume（恢复）它们
    std::cout << "[主线程] 进入调度器循环...\n\n";
    while (!co0.is_finished || !co1.is_finished) {
        
        if (!co0.is_finished) {
            std::cout << "[调度器] 准备恢复 [协程 0]...\n";
            current_id = 0;
            // 💡 核心对调：保存主线程上下文，恢复协程 0 的寄存器状态（CPU 瞬间跳进 func_A）
            swapcontext(&main_ctx, &co0.ctx); 
            std::cout << "[调度器] [协程 0] 暂停返回了，控制权回到主线程。\n\n";
        }

        if (!co1.is_finished) {
            std::cout << "[调度器] 准备恢复 [协程 1]...\n";
            current_id = 1;
            // 💡 核心对调：保存主线程上下文，恢复协程 1 的寄存器状态（CPU 瞬间跳进 func_B）
            swapcontext(&main_ctx, &co1.ctx);
            std::cout << "[调度器] [协程 1] 暂停返回了，控制权回到主线程。\n\n";
        }
    }

    std::cout << "[主线程] 所有协程执行完毕，退出程序。\n";
    return 0;
}
