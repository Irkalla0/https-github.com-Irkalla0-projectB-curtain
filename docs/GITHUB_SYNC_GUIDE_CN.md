# GitHub 同步说明（项目B）

## 1. 初始化仓库

```bash
git init
git add .
git commit -m "bootstrap: add project B total plan and voice control gateway"
```

## 2. 连接远程仓库

```bash
git remote add origin <你的仓库URL>
git branch -M main
git push -u origin main
```

示例 URL：

- HTTPS: `https://github.com/<user>/<repo>.git`

## 3. 后续更新

```bash
git add .
git commit -m "weekX: <本周功能>"
git push
```

## 4. 建议提交节奏

- 里程碑提交：每周一次
- 重大功能提交：语音模块、协议、安全、验收报告分别提交

