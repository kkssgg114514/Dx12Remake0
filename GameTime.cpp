#include "GameTime.h"

GameTime::GameTime()
	:mSencondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0),
	mPauseTime(0), mStopTime(0), mPrevTime(0), mCurrentTime(0), isStoped(false)
{
	__int64 countsPerSec;

}

float GameTime::TotalTime() const
{
	return 0.0f;
}

float GameTime::DeltaTime() const
{
	return 0.0f;
}

bool GameTime::IsStoped()
{
	return false;
}

void GameTime::Reset()
{
}

void GameTime::Start()
{
}

void GameTime::Stop()
{
}

void GameTime::Tick()
{
}
