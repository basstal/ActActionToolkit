﻿#include "Common/NovaDataBinding.h"

TMap<FName, TSharedRef<IDataBinding>> NovaDataBinding::DataBindingMap = TMap<FName, TSharedRef<IDataBinding>>();

TSharedPtr<IDataBinding> NovaDataBinding::Get(FName InName)
{
	auto ValueTypePtr = NovaDataBinding::DataBindingMap.Find(InName);
	if (ValueTypePtr)
	{
		return *ValueTypePtr;
	}
	return nullptr;
}

bool NovaDataBinding::UnBind(FName InName, FDelegateHandle InDelegateHandle)
{
	auto DataBinding = NovaDataBinding::Get(InName);
	return DataBinding->UnBind(InDelegateHandle);
}

bool NovaDataBinding::Delete(FName InName)
{
	auto ValueTypePtr = NovaDataBinding::DataBindingMap.Find(InName);
	if (ValueTypePtr)
	{
		TSharedRef<IDataBinding> DataBindingRef = *ValueTypePtr;
		DataBindingRef->Release();
		NovaDataBinding::DataBindingMap.FindAndRemoveChecked(InName);
		return true;
	}
	return false;
}

void NovaDataBinding::Trigger(FName InName, const FDelegateHandle& InDelegateHandle)
{
	if (auto DataBinding = Get(InName))
	{
		DataBinding->Trigger(InDelegateHandle);
	}
}
