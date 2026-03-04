// Copyright Project Mordecai. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

class FMordecaiCoreRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
