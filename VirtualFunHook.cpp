// VirtualFunHook.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "VirtualFunHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

// 定义target类
class target
{
public:
    virtual int Add(int a, int b);
    virtual void g()
    {
        cout << "target::g" << endl;
    }
    virtual void h()
    {
        cout << "target::h" << endl;
    }
    void novirtual()
    {
        cout << "target::not virtual" << endl;
    }
};

int target::Add(int a, int b)
{
    printf("target:Add\n");
    return a + b;
}


// Detour 类
class DetourClass
{
public:
    virtual int DetourFun(int a, int b);
};

// Trampoline 类
class TrampolineClass
{
public:
    virtual int TrampolineFun(int a, int b)
    {
        printf("TrampolineClass\n");
        return 0;
    }
};

int DetourClass::DetourFun(int a, int b)
{
    TrampolineClass* pTrampoline = new TrampolineClass;
    int result = pTrampoline->TrampolineFun(a, b);
    printf("DetourClass::OriginalFun returned %d\n", result);
    result += 10;


    delete pTrampoline;
    return result;
}

// 获得类虚拟成员函数指针
LPVOID GetClassVirtualFnAddress(LPVOID pthis, int Index)
{
    ULONG_PTR* vfTable = (ULONG_PTR*)*(ULONG_PTR*)pthis;
    return (LPVOID)vfTable[Index];

}

TrampolineClass Trampoline;                   // 创建一个TrampolineClass 类的实例对象
DetourClass Detour;                           // 创建一个DetourClass 类的实例对象
void HookClassMemberByAnotherClassMember()
{
    target b;
    target* ptarget = &b;
    MEMORY_BASIC_INFORMATION mbi;

    DWORD dwOLD;

    printf("ptarget = 0x%X\n", ptarget);

    // 虚函数表的地址
    ULONG_PTR* vfTableToHook = (ULONG_PTR*)*(ULONG_PTR*)ptarget;
    printf("vfTable = 0x%x\n", vfTableToHook);

    // 获取TrampolineClass 对象的虚函数地址表
    // 通过一系列的转换和取地址操作，vfTableTrampoline 被设置为指向虚函数表的指针
    // &Trampoline：取得Trampoline对象的地址。
    // *(ULONG_PTR*)&Trampoline：将Trampoline对象的地址强制转换为ULONG_PTR*类型的指针，然后再通过解引用操作取得其指向的内容，也就是一个指向虚函数表的指针。
    // (ULONG_PTR*)*(ULONG_PTR*)&Trampoline：将上一步得到的虚函数表指针强制转换为ULONG_PTR*类型的指针，这样vfTableTrampoline就指向了Trampoline对象的虚函数表。
    ULONG_PTR* vfTableTrampoline = (ULONG_PTR*)*(ULONG_PTR*)&Trampoline;

    // 第0步：将原函数的地址保存到当前类的表中，作为调用原函数的入口
    // 先将原函数的地址保存到当前类的表中，作为调用原函数的入口
    VirtualQuery(vfTableTrampoline, &mbi, sizeof(mbi));     // 查询虚函数表的内存信息
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &dwOLD);
    // 保存原始数据
    // 第1步：修改 Trampoline 的 虚函数表，原函数位于第几个这里就是第几个，位置必须一样
    // 替换 TrampolineClass 的成员函数的指针为 TargetFun 的地址
    vfTableTrampoline[0] = (ULONG_PTR)GetClassVirtualFnAddress(ptarget, 0);
    printf("target::Add() %p\n", vfTableTrampoline[0]);
    TrampolineClass* p = &Trampoline;
    // 恢复内存页的属性
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, dwOLD, 0);



    // 修改内存页的属性
    VirtualQuery(vfTableToHook, &mbi, sizeof(mbi));
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &dwOLD);
    // 保存原始数据
    // 第2步：修改目标类的虚函数表，修改TargetClass的虚函数表，替换其成员函数TargetFun的指针为DetourFun的地址
    vfTableToHook[0] = (ULONG_PTR)GetClassVirtualFnAddress(&Detour, 0);
    printf("Detour::Add() %p\n", vfTableToHook[0]);



    // 恢复内存页的属性
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, dwOLD, 0);
    int result = ptarget->Add(1, 2);
    printf("result = %d \nafter call member fun.\n", result);
}


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            HookClassMemberByAnotherClassMember();
            getchar();
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
