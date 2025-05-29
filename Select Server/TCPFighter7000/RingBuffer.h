#ifndef __RINGBUFFER__
#define __RINGBUFFER__



class RingBuffer
{
public:
	enum sz_RingBuffer
	{
		szRingBuffer_DEFAULT = 3000
	};

	RingBuffer();
	RingBuffer(int size);
	~RingBuffer() noexcept;

	RingBuffer(const RingBuffer& ref);
	RingBuffer& operator=(const RingBuffer& ref);


	void Resize(int size);
	int GetBufferSize(void) const noexcept; // ���밡���� ũ��(bufferSize - 1)�� ����, bufferSize ���� ���� ���� ���
	int GetUseSize(void) const noexcept; // ���� ������� �뷮 ���
	int GetFreeSize(void) const noexcept;// ���� ���ۿ� ���� ũ�� ���

	int Enqueue(char* data, int size) noexcept;
	int Dequeue(char* dest, int size) noexcept;
	int Peek(char* dest, int size) noexcept;
	void ClearBuffer(void) noexcept;

	int DirectEnqueueSize(void) const noexcept; // tail - ring buffer�� �������� ũ�� Ȥ�� tail - head�� ũ��
	int DirectDequeueSize(void) const noexcept; // head - tail�� ũ�� Ȥ�� head - ring buffer�� �������� ũ��

	int MoveHead(int size) noexcept; // GetHeadBufferPtr�� ����
	int MoveTail(int size) noexcept; // GetTailBufferPtr�� ����

	char* GetHeadBufferPtr(void) noexcept; // for delete
	char* GetTailBufferPtr(void) noexcept; // for insert
	char* GetRingBufferStartPtr(void) noexcept;
private:
	int bufferSize = 4000;
	char* inBuffer;
	size_t head = 0;
	size_t tail = 0;

};
#endif