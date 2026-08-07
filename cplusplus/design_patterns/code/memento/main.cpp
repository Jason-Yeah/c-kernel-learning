#include "caretaker.hpp"
#include "originator.hpp"

int main()
{
    Editor editor;
    History history;

    editor.Type("Hello");
    editor.Show();

    editor.Type(" World");
    editor.Show();
    std::cout << "\n=== 保存快照 ===\n";

    history.Push(editor.CreatrMemento());
    editor.Type("!!!");
    editor.MoveCursor(0);
    editor.Show();

    std::cout << "\n=== 撤销 ===\n";
    auto snapshot = history.Pop();
    if (snapshot)
    {
        editor.Restore(*snapshot);
    }
    editor.Show();

    return 0;
}