#include "GameTime.h"

GameTime::GameTime()
	:mSencondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0),
	mPauseTime(0), mStopTime(0), mPrevTime(0), mCurrentTime(0), isStoped(false)
{
	//���������ÿ����ٴ�
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSencondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTime::TotalTime() const
{
	if (isStoped)
	{
		//��ǰ����ֹͣ״̬
		//��ֹͣʱ�̵�ʱ���ȥ֮ǰ��ͣ����ʱ��
		return (float)((mStopTime - mPauseTime - mBaseTime) * mSencondsPerCount);
	}
	else
	{
		//���������ͣ״̬
		//�õ�ǰʱ�̵�ʱ���ȥ��ͣ��ʱ��
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

	mBaseTime = currTime;		//��ǰʱ����Ϊ��׼ʱ��
	mPrevTime = currTime;		//��ǰʱ��Ҳ����һ֡ʱ�䣬��Ϊû����һ֡������ʱ����ǵ�һ֡��
	mStopTime = 0;				//���õ���һ��ֹͣʱ��Ϊ0
	isStoped = false;			//����ֹͣ����
}

void GameTime::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	if (isStoped)
	{
		//��ֹͣ״̬���
		//�������ͣ��ʱ�䣨������ͣʱ���������
		mPauseTime += (startTime - mStopTime);
		//�޸�ֹͣ״̬
		mPrevTime = startTime;	//������һ֡
		mStopTime = 0;			//����ֹͣʱ��
		isStoped = false;		//�޸�ֹͣ״̬
	}
	//����������о�ʲô������
}

void GameTime::Stop()
{
	//�ж��Ƿ��Ѿ�ֹͣ�������Ѿ�ֹͣ����ʲô������
	if (!isStoped)
	{
		//����δֹͣ
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		mStopTime = currTime;			//����ǰʱ����Ϊֹͣ��һ�̵�ʱ��
		isStoped = true;				//����ֹͣ״̬
	}
}

void GameTime::Tick()
{
	if (isStoped)
	{
		//�Ѿ���ͣ
		mDeltaTime = 0.0f;
		return;
	}
	//���㵱ǰʱ�̵ļ���ֵ
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	mCurrentTime = currentTime;
	//���㵱ǰ֡��ǰһ֡��ʱ��������ֵ * һ����ʱ���룩
	mDeltaTime = (mCurrentTime - mPrevTime) * mSencondsPerCount;
	//��ǰ����ֵ�ѹ��ڣ���Ϊǰһ֡����ֵ
	mPrevTime = mCurrentTime;
	//�ų�ʱ���Ϊ��ֵ
	if (mDeltaTime < 0)
	{
		mDeltaTime = 0.0f;
	}
}
