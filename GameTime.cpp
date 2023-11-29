#include "GameTime.h"

GameTime::GameTime()
	:mSencondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0),
	mPauseTime(0), mStopTime(0), mPrevTime(0), mCurrentTime(0), isStoped(false)
{
	//计算计数器每秒多少次
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSencondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTime::TotalTime() const
{
	if (isStoped)
	{
		//当前处于停止状态
		//用停止时刻的时间减去之前暂停的总时间
		return (float)((mStopTime - mPauseTime - mBaseTime) * mSencondsPerCount);
	}
	else
	{
		//如果不在暂停状态
		//用当前时刻的时间减去暂停总时间
		return (float)((mCurrentTime - mPauseTime - mBaseTime) * mSencondsPerCount);
	}
}

float GameTime::DeltaTime() const
{
	return (float)mDeltaTime;
}

bool GameTime::IsStoped()
{
	return isStoped;
}

void GameTime::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;		//当前时间作为基准时间
	mPrevTime = currTime;		//当前时间也是上一帧时间，因为没有上一帧（重置时候就是第一帧）
	mStopTime = 0;				//重置的那一刻停止时间为0
	isStoped = false;			//永不停止运行
}

void GameTime::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	if (isStoped)
	{
		//从停止状态解除
		//计算出暂停的时间（所有暂停时间加起来）
		mPauseTime += (startTime - mStopTime);
		//修改停止状态
		mPrevTime = startTime;	//重置上一帧
		mStopTime = 0;			//重置停止时间
		isStoped = false;		//修改停止状态
	}
	//如果正常运行就什么都不做
}

void GameTime::Stop()
{
	//判断是否已经停止，若是已经停止，则什么都不做
	if (!isStoped)
	{
		//若是未停止
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		mStopTime = currTime;			//将当前时间作为停止那一刻的时间
		isStoped = true;				//设置停止状态
	}
}

void GameTime::Tick()
{
	if (isStoped)
	{
		//已经暂停
		mDeltaTime = 0.0f;
		return;
	}
	//计算当前时刻的计数值
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	mCurrentTime = currentTime;
	//计算当前帧和前一帧的时间差，记数差值 * 一次用时（秒）
	mDeltaTime = (mCurrentTime - mPrevTime) * mSencondsPerCount;
	//当前计数值已过期，成为前一帧计数值
	mPrevTime = mCurrentTime;
	//排除时间差为负值
	if (mDeltaTime < 0)
	{
		mDeltaTime = 0.0f;
	}
}
