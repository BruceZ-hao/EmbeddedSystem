@echo off
:: 改用GBK编码（适配中文系统默认ANSI，彻底解决乱码）
chcp 936 >nul 2>&1

:: 固定仓库目录
set "repo_dir=C:\EmbeddedSystem"

:: 检查目录是否存在
if not exist "%repo_dir%" (
    echo 错误：未找到仓库目录 %repo_dir%
    echo 请先执行克隆脚本将仓库下载到C盘！
    pause
    exit /b 1
)

echo 进入仓库目录：%repo_dir%
cd /d "%repo_dir%" || (
    echo 错误：无法进入目录 %repo_dir%
    pause
    exit /b 1
)

echo 拉取最新代码...
git pull origin main

if %errorlevel% equ 0 (
    echo ==================================
    echo 代码更新完成！当前已是最新版本。
    echo ==================================
) else (
    echo ==================================
    echo 拉取失败！请检查：
    echo 1. 本地代码是否有未提交的修改冲突
    echo 2. 网络是否能访问GitHub（可尝试代理）
    echo 3. 仓库分支是否为master（需替换脚本中的main）
    echo ==================================
)
pause