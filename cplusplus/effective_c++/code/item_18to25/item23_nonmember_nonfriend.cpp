#include <iostream>

class WebBrowser
{
public:
    void clearCache() { std::cout << "clear cache\n"; }
    void clearHistory() { std::cout << "clear history\n"; }
    void removeCookies() { std::cout << "remove cookies\n"; }
};

// 不是成员、也不是 friend；它只使用 WebBrowser 的 public 接口。
void clearEverything(WebBrowser &browser)
{
    browser.clearCache();
    browser.clearHistory();
    browser.removeCookies();
}

int main()
{
    WebBrowser browser;
    clearEverything(browser);
}
