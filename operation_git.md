# GIT初始化

## 1. 进入项目目录

`cd ~/your_project_path`

## 2. 初始化 Git 仓库

`git init`

## 3. 关联远程仓库 (注意：已使用最新的用户名 Jason-Yeah)

`git remote add origin git@github.com:Jason-Yeah/c-kernel-learning.git`

## 4. 创建并切换到 main 分支

`git branch -M main`

# GIT日常推送

## 1. 所有改动放到暂存区

`git add .`

## 2. 提交到本地仓库，记录说明

`git commit -m "feat: 补充了xxx"`

## 3. 推送到GitHub

`git push origin main`

# GIT删内容

## 直接在文件夹删除文件

```bash
# 假设要删除 test.c
rm test.c
git add .
git commit -m "fix: 删除冗余的测试代码"
git push
```

## 仅删除云端

```bash
# --cached 参数表示只从 Git 追踪里移除，不删本地物理文件
git rm -r --cached build/
git commit -m "chore: 从仓库中移除编译产物"
git push
```

# GIT仓库干净

使用`.gitignore`文件过滤

```text
# 编译产物
*.o
*.out
build/
bin/

# 编辑器配置
.vscode/
.idea/

# 系统临时文件
.DS_Store
```