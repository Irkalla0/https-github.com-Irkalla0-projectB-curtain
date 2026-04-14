# GitHub 同步说明（项目B）

## 1. 常规推送

```bash
git add <files>
git commit -m "your message"
git push origin main
```

## 2. 如果 Windows 直连 443 失败

改用 WSL 推送（常见于本机网络策略拦截）：

```bash
wsl.exe -e bash -lc "cd /mnt/d/codex/projectB && git push origin main"
```

## 3. 如果 WSL 需要调用 Windows 凭据

```bash
wsl.exe -e bash -lc "cd /mnt/d/codex/projectB && git config credential.helper '/mnt/c/PROGRA~1/Git/mingw64/bin/git-credential-manager.exe'"
```

然后重新推送。
