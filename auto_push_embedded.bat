@echo off
:: 替换原有chcp命令
chcp 936 >nul 2>&1  :: 改用GBK编码适配Windows cmd中文显示
title 嵌入式项目-新增文件夹+大文件自动化推送脚本

:: ========== 核心配置项（重点：新增文件夹配置） ==========
set "REPO_DIR=C:\EmbeddedSystem"       :: 仓库根目录
set "BRANCH=main"                      :: 推送分支
set "PROXY_IP=127.0.0.1"               :: 代理IP
set "PROXY_PORT=7897"                  :: 代理端口
set "LFS_EXT_LIST=bin hex img iso zip rar" :: LFS追踪的文件后缀
set "NEW_DIR=SomeTools"                 :: 新增的大文件文件夹名
set "COMMIT_MSG=Auto push: 新增%NEW_DIR%文件夹+大固件 %date% %time%"

:: ========== 步骤1：检查/创建新增文件夹 ==========
echo [1/9] 检查/创建%NEW_DIR%文件夹...
if not exist "%REPO_DIR%\%NEW_DIR%" (
    echo ??  %NEW_DIR%文件夹不存在，自动创建...
    mkdir "%REPO_DIR%\%NEW_DIR%"
)
cd /d "%REPO_DIR%" || (
    echo ? 无法进入仓库目录 %REPO_DIR%
    pause
    exit /b 1
)

:: ========== 步骤2：安装/初始化Git LFS ==========
echo [2/9] 初始化Git LFS...
git lfs install >nul 2>&1
if %errorlevel% neq 0 (
    echo ??  自动安装Git LFS...
    powershell -Command "Invoke-WebRequest -Uri https://github.com/git-lfs/git-lfs/releases/download/v3.5.1/git-lfs-windows-amd64-v3.5.1.zip -OutFile git-lfs.zip" >nul 2>&1
    powershell -Command "Expand-Archive git-lfs.zip -DestinationPath git-lfs-temp -Force" >nul 2>&1
    cd git-lfs-temp && git-lfs install >nul 2>&1 && cd ..
    del /q git-lfs.zip >nul 2>&1
    rmdir /s /q git-lfs-temp >nul 2>&1
)

:: ========== 步骤3：配置LFS追踪【新增文件夹内】的大文件 ==========
echo [3/9] 配置LFS追踪%NEW_DIR%文件夹下的大文件...
:: 1. 追踪指定后缀（全局）
for %%e in (%LFS_EXT_LIST%) do (
    git lfs track "*%%e" >nul 2>&1
)
:: 2. 额外追踪新增文件夹下的所有文件（兜底，确保无遗漏）
git lfs track "%NEW_DIR%/*" >nul 2>&1
:: 保存LFS配置
git add .gitattributes >nul 2>&1

:: ========== 步骤4：配置代理和SSL ==========
echo [4/9] 配置代理和SSL...
git config --local http.sslBackend openssl
git config --local http.proxy http://%PROXY_IP%:%PROXY_PORT%
git config --local https.proxy http://%PROXY_IP%:%PROXY_PORT%
git config --local http.sslVerify false

:: ========== 步骤5：拉取远程最新代码 ==========
echo [5/9] 拉取远程最新代码...
git lfs pull origin %BRANCH% >nul 2>&1
git pull origin %BRANCH% --no-edit
if %errorlevel% neq 0 (
    echo ? 拉取失败，请手动解决冲突！
    pause
    exit /b 1
)

:: ========== 步骤6：添加新增文件夹+所有文件 ==========
echo [6/9] 添加%NEW_DIR%文件夹及所有文件...
:: 优先添加新增文件夹（确保大文件被LFS识别）
git add "%NEW_DIR%/"
:: 添加仓库内其他文件
git add .

:: ========== 步骤7：检查是否有文件需提交 ==========
git diff --cached --quiet
if %errorlevel% equ 0 (
    echo ??  无文件修改（含%NEW_DIR%文件夹），跳过提交
    goto PUSH_STEP
)

:: ========== 步骤8：提交修改 ==========
echo [7/9] 提交%NEW_DIR%文件夹+大文件...
git commit -m "%COMMIT_MSG%"
if %errorlevel% neq 0 (
    echo ? 提交失败！
    pause
    exit /b 1
)

:: ========== 步骤9：推送（优先推LFS大文件） ==========
:PUSH_STEP
echo [8/9] 推送LFS大文件...
git lfs push --all origin %BRANCH%
echo [9/9] 推送代码到远程...
git push origin %BRANCH%

if %errorlevel% equ 0 (
    echo ? 推送成功！%NEW_DIR%文件夹+大文件已上传至远程仓库
) else (
    echo ? 推送失败！检查代理/LFS/文件大小（GitHub LFS上限5GB）
)

:: ========== 清理配置 ==========
git config --local http.sslVerify true
git config --local --unset http.proxy
git config --local --unset https.proxy

echo.
pause >nul
exit /b 0