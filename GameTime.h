#pragma once
#include <Windows.h>

class GameTime
{
public:
	GameTime();

	float TotalTime()const;				//#游戏运行的总时间（不包括暂停）
	float DeltaTime()const;				//#获取mDeltaTime变量
	bool IsStoped();					//#获取isStoped变量
										  
	void Reset();						//#重置计时器
	void Start();						//#开始计时器
	void Stop();						//#停止计时器
	void Tick();						//#计算每帧时间间隔
										  
private:								  
	double mSencondsPerCount;			//#计数器每一次需要多少秒
	double mDeltaTime;					//#每帧时间（前一帧和当前帧的时间差）
										  
	__int64 mBaseTime;					//#重置后的基准时间
	__int64 mPauseTime;					//#暂停的总时间
	__int64 mStopTime;					//#停止那一刻的时间
	__int64 mPrevTime;					//#上一帧时间
	__int64 mCurrentTime;				//#本帧时间
										  
	bool isStoped;						//#是否为停止的状态
};

