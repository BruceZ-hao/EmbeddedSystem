@echo off
chcp 936 >nul 2>&1
set "repo_dir=C:\EmbeddedSystem"
set "lfs_ext_list=bin hex zip iso img"  # 需要LFS追踪的大文件后缀（按需加）

:: 进入仓库目录
cd /d "%repo_dir%" || (echo [错误] 目录不存在&pause&exit /b 1)

:: 1. 初始化Git LFS（首次运行自动安装，需提前装Git）
git lfs install >nul 2>&1
if %errorlevel% neq 0 (
    echo [提示] 未安装Git LFS，自动安装...
    :: 自动下载安装Git LFS（Windows）
    powershell -Command "Invoke-WebRequest -Uri https://github.com/git-lfs/git-lfs/releases/download/v3.4.1/git-lfs-windows-amd64-v3.4.1.zip -OutFile git-lfs.zip"
    powershell -Command "Expand-Archive git-lfs.zip -DestinationPath git-lfs-temp"
    cd git-lfs-temp && git-lfs install && cd ..
    del /q git-lfs.zip && rmdir /s /q git-lfs-temp
)

:: 2. 自动化追踪所有大文件后缀
for %%e in (%lfs_ext_list%) do (
    git lfs track "*%%e" >nul 2>&1
)
git add .gitattributes >nul 2>&1  # 保存LFS追踪配置

:: 3. 分批添加文件（每批≤99个）
echo [信息] 自动化分批添加文件...
set "file_count=0"
set "batch_num=1"
for /r %%f in (*) do (
    :: 排除.git目录、临时文件
    if not "%%~dpf"=="%repo_dir%\.git\" (
        git add "%%f" >nul 2>&1
        set /a file_count+=1
        :: 每99个文件提交一批
        if !file_count! equ 99 (
            git commit -m "auto push: batch %batch_num% (99 files)"
            set /a batch_num+=1
            set "file_count=0"
        )
    )
)

:: 4. 提交剩余文件
if !file_count! gtr 0 (
    git commit -m "auto push: batch %batch_num% (!file_count! files)"
)

:: 5. 推送（LFS自动处理大文件）
echo [信息] 推送所有批次到远程仓库...
git push origin main

if %errorlevel% equ 0 (
    echo [成功] 自动化推送完成！
) else (
    echo [失败] 推送失败，可检查网络/仓库权限
)
pause