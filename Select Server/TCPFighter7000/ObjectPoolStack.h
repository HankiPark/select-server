#pragma once
#include <iostream>
#include <vector>

static int globalTemplateCnt = 1;

// ���� ��, �޸� Ǯ�� ��� (alloc�Ǿ��ִ�) �޸� �����ǵ��� ����
// �⺻������ ��� �ΰ� ����� ������� �������� ������
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
		printf("Object Pool���� %d���� ��ü�� �־���, �� %d���� ��ü�� �Ҵ������߽��ϴ�. \n", this->now, this->total);
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
		//�� ����ߴٸ� ���� ������
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

	//��� ��� ����
	
	bool toUseReplacementFlag = true; // replacement new ��� ���� / �ܺο��� ��ü�� �˾Ƽ� �ٷ� ��
	bool initConstructionFlag = true; // �ʱ� �Ҵ�� ������ ȣ�� ����
	bool toUseAddressList = false; // addresslist ��� ����
	bool toUseSafeFlag = false; // safe �� ��� ����
	bool toUseFreeFlag = false; // alloc, free ǥ�� ���� 
	bool toUseUniqueId = false; // unique id ��� ����

	//�޸� Ǯ�� �ִٸ� true, ���ٸ� false
	bool isFree = true; 
	int frontSafe = 0xDEC0ADDE; // 0xDEADC0DE
	int rearSafe = 0x01F0BABA; // 0xBABAF001
	int uniqueId = 0; // �ܺο��� ���� �ο�
	//int additionalDataSize;
	std::vector<uintptr_t> addressList;

	Node* _node = NULL;
};