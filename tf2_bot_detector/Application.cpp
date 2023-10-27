#include "Application.h"
#include "DB/TempDB.h"
#include "UI/MainWindow.h"

#include <mh/error/ensure.hpp>

#include <cassert>

using namespace tf2_bot_detector;

static TF2BDApplication* s_Application;

TF2BDApplication::TF2BDApplication()
{
	assert(!s_Application);
	s_Application = this;

	m_TempDB = DB::ITempDB::Create();
}

TF2BDApplication::~TF2BDApplication() = default;

TF2BDApplication& TF2BDApplication::GetApplication()
{
	return *mh_ensure(s_Application);
}

DB::ITempDB& TF2BDApplication::GetTempDB()
{
	return *mh_ensure(m_TempDB.get());
}
