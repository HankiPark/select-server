#pragma once
#include <iostream>
#include <vector>

static int globalTemplateCnt = 1;

// 해제 시, 메모리 풀의 모든 (alloc되어있는) 메모리 해제되도록 설정
// 기본적으로 모든 부가 기능은 사용하지 않음으로 설정됨
template <typename T>
class ObjectPool
{
public:
#pragma pack(push, 1)
	struct Node
	{
		int frontSafe;
		char data[sizeof(T)];
		Node* next;
		int rearSafe;
		int uniqueId;
		bool isFree;
		Node(int front, int rear, int id)
		{
			frontSafe = front;
			next = NULL;
			rearSafe = rear;
			uniqueId = id;
			isFree = false;
		}
	};
#pragma pack(pop)
	ObjectPool() 
	{
		uniqueId = globalTemplateCnt++;
	};

	ObjectPool(bool toUseReplacement, bool initConstruction = true, bool toUseAddressList = false,
		bool toUseSafeFlag = false, bool toUseFree = false, bool toUseUniqueId = false)
	{
		this->initConstructionFlag = initConstruction;
		this->toUseReplacementFlag = toUseReplacement;
		this->toUseAddressList = toUseAddressList;
		this->toUseSafeFlag = toUseSafeFlag;
		this->toUseFreeFlag = toUseFree;
		this->toUseUniqueId = toUseUniqueId;
		uniqueId = globalTemplateCnt++;
	};
	~ObjectPool() 
	{

		if (!toUseReplacementFlag)
		{
			for (int i = 0; i < this->addressList.size(); i++)
			{
				delete (reinterpret_cast<T*>(this->addressList[i]));
			}
		}
		else
		{
			for (int i = 0; i < this->addressList.size(); i++)
			{
				free(reinterpret_cast<T*>(this->addressList[i]));
			}
		}
		printf("Object Pool에는 %d개의 객체가 있었고, 총 %d개의 객체를 할당해제했습니다. \n", this->now, this->total);
	};

	T* Alloc()
	{
		//printf("size : %d \n", sizeof(T));
		if (_node == NULL)
		{
			total++;
			Node* n = new Node(this->frontSafe, this->rearSafe, this->uniqueId);
			if (this->toUseAddressList)
			{
				this->addressList.push_back(reinterpret_cast<uintptr_t>(n));
			}
			if (this->initConstructionFlag)
			{
				new (&(n->data)) T;
			}

			return reinterpret_cast<T*> (reinterpret_cast<char*>(n) + sizeof(this->frontSafe));
		}
		Node* temp = _node;
		_node = _node->next;
		temp->next = nullptr;
		this->now--;
		if (!this->toUseReplacementFlag)
		{
			new(&(temp->data)) T;
		}
		return reinterpret_cast<T*> (reinterpret_cast<char*>(temp) + sizeof(this->frontSafe));

	}
	bool Free(T*& t)
	{
		Node* temp = reinterpret_cast<Node*>(reinterpret_cast<char*>(t) - sizeof(this->frontSafe));

		if (this->toUseFreeFlag)
		{
			if (temp->isFree == true)
			{
				//error
				__debugbreak();
			}
		}
		if (this->toUseUniqueId)
		{
			if (temp->uniqueId != this->uniqueId)
			{
				//error
				__debugbreak();
			}
		}
		if (this->toUseSafeFlag)
		{
			if (temp->frontSafe != this->frontSafe)
			{
				//error
				__debugbreak();
			}
			if (temp->rearSafe != this->rearSafe)
			{
				//error
				__debugbreak();
			}
		}
		//다 통과했다면 정상 데이터
		if (!this->toUseReplacementFlag)
		{
			t->~T();
		}
		this->now++;
		t = nullptr;
		if (_node == NULL)
		{
			_node = temp;
			return true;
		}
		temp->next = _node;
		_node = temp;
		
		return true;
	}

private:
	int total = 0;
	int now = 0;

	//기능 사용 여부
	
	bool toUseReplacementFlag = true; // replacement new 사용 여부 / 외부에서 객체를 알아서 다룰 것
	bool initConstructionFlag = true; // 초기 할당시 생성자 호출 여부
	bool toUseAddressList = false; // addresslist 사용 여부
	bool toUseSafeFlag = false; // safe 값 사용 여부
	bool toUseFreeFlag = false; // alloc, free 표기 여부 
	bool toUseUniqueId = false; // unique id 사용 여부

	//메모리 풀에 있다면 true, 없다면 false
	bool isFree = true; 
	int frontSafe = 0xDEC0ADDE; // 0xDEADC0DE
	int rearSafe = 0x01F0BABA; // 0xBABAF001
	int uniqueId = 0; // 외부에서 전역 부여
	//int additionalDataSize;
	std::vector<uintptr_t> addressList;

	Node* _node = NULL;
};