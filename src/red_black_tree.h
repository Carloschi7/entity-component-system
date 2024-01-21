#pragma once
#include <type_traits>
#include "types.h"

//Store contiguously data of a specific type
template <class Type>
class BucketAllocator
{
	struct BucketChain;
	static_assert(std::is_default_constructible_v<Type>, "Type needs to be default constructible");
	//static_assert(std::is_default_destructible_v<Type>, "Type needs to be default constructible");
public:
	BucketAllocator() = default;

	~BucketAllocator() 
	{
		_destroy(internal_storage);
	}

	void* allocate_new()
	{
		if (internal_storage == nullptr) {
			capacity = BUCKET_SIZE;
			size = 0;
			internal_storage = static_cast<BucketChain*>(::operator new(sizeof(BucketChain)));
			internal_storage->next = nullptr;
			std::memset(internal_storage, 0, sizeof(BucketChain));
		}

		if (size >= capacity) {
			capacity += BUCKET_SIZE;
			
			BucketChain* iter = internal_storage;
			while (iter->next) iter = iter->next;
			iter->next = new BucketChain;
			iter->next->next = nullptr;
		}

		++size;
		u32 index = 0;
		BucketChain* iter = internal_storage;
		for (; index < capacity; index++) {
			if (index != 0 && index % BUCKET_SIZE == 0) {
				iter = iter->next;
				assert(iter != nullptr);
				index %= 64;
			}

			if (iter->bucket[index].memory_state != MEMORY_USED)
				break;
		}

		if(iter->bucket[index].memory_state & MEMORY_DIRTY)
			std::memset(&iter->bucket[index].allocated, 0, sizeof(Type));

		iter->bucket[index].memory_state = MEMORY_USED;
		return &iter->bucket[index].allocated;
	}

	void free(void* memory, bool clear_mem = false)
	{
		BucketChain* iter = internal_storage;
		for (u32 index = 0; index < capacity; index++) {
			if (index != 0 && index % BUCKET_SIZE == 0) {
				iter = iter->next;
				assert(iter != nullptr);
				index %= 64;
			}

			if (&iter->bucket[index].allocated == memory) {
				if (clear_mem) {
					std::memset(&iter->bucket[index].allocated, 0, sizeof(Type));
					iter->bucket[index].memory_state = MEMORY_NOT_USED;
				}
				else {
					iter->bucket[index].memory_state = MEMORY_DIRTY;
				}
				break;
			}
		}
	}

private:


	void _destroy(BucketChain* chain) 
	{
		if (chain == nullptr)
			return;

		_destroy(chain->next);
		::operator delete[](chain);
	}

private:
	static constexpr u8 MEMORY_NOT_USED = 0x00;
	static constexpr u8 MEMORY_USED     = 0x01;
	static constexpr u8 MEMORY_DIRTY    = 0x02;
	static constexpr u8 BUCKET_SIZE     = 64;

	struct BucketValue
	{
		u8 memory_state;
		Type allocated;
	};

	struct BucketChain
	{
		BucketValue bucket[BUCKET_SIZE];
		BucketChain* next = nullptr;
	};

	BucketChain* internal_storage = nullptr;
	u32 capacity = 0, size = 0;


};

enum class Color { Black = 0, Red };

template<typename Type, typename Index = u64>
struct TreeNode
{
	Color color;
	Index index;
	Type* content;
	TreeNode* left, * right, * parent;

	TreeNode* find_next()
	{
		TreeNode* node = this;
		if (node->right) {
			TreeNode* sub_node = node->right;
			while (sub_node->left)
				sub_node = sub_node->left;

			return sub_node;
		}

		TreeNode* sub_node = node->parent;
		while (sub_node && sub_node->index < index)
			sub_node = sub_node->parent;

		return sub_node;
	}

	TreeNode* find_prev()
	{
		TreeNode* node = this;
		if (node->left) {
			TreeNode* sub_node = node->left;
			while (sub_node->right)
				sub_node = sub_node->right;

			return sub_node;
		}

		TreeNode* sub_node = node->parent;
		while (sub_node && sub_node->index > index)
			sub_node = sub_node->parent;

		return sub_node;
	}
};

template<typename Type, typename Index = u64, class Allocator = BucketAllocator<TreeNode<Type, Index>>>
class RedBlackTree
{
	static_assert(std::is_integral_v<Index>, "Index needs to be of integral type");
	using NodeType = TreeNode<Type, Index>;
public:
	RedBlackTree() : root{ nullptr }, node_count{0} {}

	~RedBlackTree()
	{
		destroy_tree(root);
	}

	template<class... Args>
	Type& emplace_node(Index value, Args&&... args)
	{
		static_assert(std::is_constructible_v<Type, Args...>, "The type needs to be constructible with the given params");
		++node_count;

		if (root == nullptr) {
			root = static_cast<NodeType*>(alloc.allocate_new());

			root->color = Color::Black;
			root->index = value;
			root->content = new Type{ std::forward<Args>(args)... };
			return *root->content;
		}

		NodeType* node = nullptr;

		{
			NodeType** _iter = &root;
			NodeType* _parent = nullptr;
			while (*_iter != nullptr) {
				if (value > (*_iter)->index) {
					_parent = *_iter;
					_iter = &(*_iter)->right;
				}
				else {
					_parent = *_iter;
					_iter = &(*_iter)->left;
				}
			}

			*_iter = static_cast<NodeType*>(alloc.allocate_new());
			(*_iter)->color = Color::Red;
			(*_iter)->parent = _parent;
			(*_iter)->index = value;
			(*_iter)->content = new Type{ std::forward<Args>(args)... }; 

			node = *_iter;
		}

		//No need to fixup
		if (node->parent->color == Color::Black)
			return *node->content;

		emplace_node_fixup(node);
		return *node->content;
	}

	NodeType* find_node(Index index)
	{
		NodeType* node = root;
		while (node != nullptr && node->index != index) {
			if (node->index < index)
				node = node->right;
			else
				node = node->left;
		}

		return node;
	}

	NodeType* find_first() 
	{
		assert(root != nullptr);
		NodeType* node = root;
		while (node->left)
			node = node->left;

		return node;
	}

	NodeType* find_last()
	{
		assert(root != nullptr);
		NodeType* node = root;
		while (node->right)
			node = node->right;

		return node;
	}

	void delete_node(Index index)
	{
		NodeType* node = root;
		while(node == nullptr || node->index != index){
			if(node->index < index)
				node = node->right;
			else
				node = node->left;
		}

		delete_node(node);
	}

	void delete_node(NodeType* node)
	{
		if (node == nullptr)
			return;

		isolate_node(node);
		delete_node_fixup(node);
		cleanup_terminal_node(node);

		if (--node_count == 0)
			root = nullptr;
	}

private:
	void isolate_node(NodeType*& node)
	{
		if (node->left == nullptr && node->right == nullptr) {
			return;
		}
		else if (node->left == nullptr || node->right == nullptr) {
			if (node->left != nullptr) {
				swap_node_data(node, node->left);
				node = node->left;
			}
			else {
				swap_node_data(node, node->right);
				node = node->right;
			}

			return;
		}
		else {
			//Node has both children
			NodeType* iter = node->left;
			while (iter->right)
				iter = iter->right;

			swap_node_data(node, iter);
			node = iter;
		}
	}

	void emplace_node_fixup(NodeType* node)
	{
		//Insertion case 3
		assert(node->parent != nullptr);
		Color uncle_color = uncle_node(node) != nullptr ? uncle_node(node)->color : Color::Black;
		Color father_color = node->parent->color;

		if (father_color == Color::Red && uncle_color == Color::Red) {
			//uncle exists if it is red
			grandfather_node(node)->color = Color::Red;
			uncle_node(node)->color = Color::Black;
			node->parent->color = Color::Black;

			if (grandfather_node(node->parent) == nullptr)
				return;

			//Check if the generated red gives problems
			node = node->parent;
			uncle_color = uncle_node(node) != nullptr ? uncle_node(node)->color : Color::Black;
			father_color = node->parent->color;
		}

		if (father_color == Color::Red && uncle_color == Color::Black) {
			//Insertion case 4
			if (node->parent->right == node && node->parent == grandfather_node(node)->left) {
				rotate_left(node);
				node = node->left;
			}
			if (node->parent->left == node && node->parent == grandfather_node(node)->right) {
				rotate_right(node);
				node = node->right;
			}

			//Insertion case 5
			node->parent->color = Color::Black;
			grandfather_node(node)->color = Color::Red;
			if (node->parent->left == node && node->parent == grandfather_node(node)->left) {
				rotate_right(node->parent);
			}
			if (node->parent->right == node && node->parent == grandfather_node(node)->right) {
				rotate_left(node->parent);
			}
		}
	}

	void delete_node_fixup(NodeType* node)
	{
		if (node->parent == nullptr)
			return;

		NodeType* parent = node->parent;
		NodeType* sibling = sibling_node(node);
		if (parent->left == node)
			parent->left = nullptr;
		else
			parent->right = nullptr;


		if (node->color == Color::Red)
			return;
		else if (node->color == Color::Black && parent->color == Color::Red) {
			if (!sibling)
				parent->color = Color::Black;
			else {
				if (sibling == parent->right)
					rotate_left(sibling);
				else
					rotate_right(sibling);
			}
		}
		else {
			//Both black
			assert(sibling != nullptr);
			if (sibling->color == Color::Black) {
				if (sibling->left != nullptr && sibling->left->color == Color::Red &&
					parent->left == sibling) {
					sibling->left->color = Color::Black;
					rotate_right(sibling);
				}

				if (sibling->right != nullptr && sibling->right->color == Color::Red &&
					parent->right == sibling) {
					sibling->right->color = Color::Black;
					rotate_left(sibling);
				}

				if (sibling->left != nullptr && sibling->left->color == Color::Red &&
					parent->right == sibling) {
					sibling->left->color = Color::Black;
					rotate_left(sibling->left);
					rotate_right(sibling->left);
				}

				if (sibling->right != nullptr && sibling->right->color == Color::Red &&
					parent->left == sibling) {
					sibling->right->color = Color::Black;
					rotate_left(sibling->right);
					rotate_right(sibling->right);
				}

				if ((sibling->right == nullptr || sibling->right->color == Color::Black) &&
					(sibling->left == nullptr || sibling->left->color == Color::Black)) {
					sibling->color = Color::Red;
				}
			}
			else if (sibling->color == Color::Red) {
				sibling->color = Color::Black;
				if (parent->right == sibling) {
					if (sibling->left) sibling->left->color = Color::Red;
					rotate_left(sibling);
				}
				else {
					if (sibling->right) sibling->right->color = Color::Red;
					rotate_right(sibling);
				}
			}
		}
	}

	void cleanup_terminal_node(NodeType* node)
	{
		delete node->content;
		alloc.free(node);
	}

	void swap_node_data(NodeType* first, NodeType* second)
	{
		Type* tmp = first->content;
		first->content = second->content;
		second->content = tmp;

		Index index = first->index;
		first->index = second->index;
		second->index = index;
	}

	void rotate_left(NodeType* node)
	{
		NodeType* parent = node->parent;
		NodeType* grandfather = grandfather_node(node);
		assert(parent != nullptr);

		if (grandfather != nullptr) {
			if (grandfather->right == parent)
				grandfather->right = node;
			else
				grandfather->left = node;
		}
		else {
			root = node;
		}

		node->parent = grandfather;
		NodeType* node_left_branch = node->left;
		node->left = parent;
		parent->parent = node;

		parent->right = node_left_branch;
		if(node_left_branch != nullptr) node_left_branch->parent = parent;
	}

	void rotate_right(NodeType* node)
	{
		NodeType* parent = node->parent;
		NodeType* grandfather = grandfather_node(node);
		assert(parent != nullptr);

		if (grandfather != nullptr) {
			if (grandfather->right == parent)
				grandfather->right = node;
			else
				grandfather->left = node;
		}
		else {
			root = node;
		}

		node->parent = grandfather;
		NodeType* node_right_branch = node->right;
		node->right = parent;
		parent->parent = node;

		parent->left = node_right_branch;
		if (node_right_branch != nullptr) node_right_branch->parent = parent;
	}

	inline NodeType* grandfather_node(NodeType* node) 
	{
		assert(node->parent != nullptr);
		return node->parent->parent;
	}

	inline NodeType* uncle_node(NodeType* node)
	{
		NodeType* gfather = grandfather_node(node);
		assert(gfather != nullptr);

		return node->parent == gfather->right ? gfather->left : gfather->right;
	}

	inline NodeType* sibling_node(NodeType* node)
	{
		assert(node->parent != nullptr);
		if(node == node->parent->right)
			return node->parent->left;
		else
			return node->parent->right;
	}

private:
	void destroy_tree(NodeType* node)
	{
		if (node == nullptr)
			return;

		destroy_tree(node->left);
		destroy_tree(node->right);
		
		if (node->content) {
			node->content->~Type();
			delete node->content;
		}
		
		//alloc.free(Node);
		//dont bother with the deletion, we free everything at the end
	}

private:
	NodeType* root;
	u32 node_count;
	Allocator alloc;
};
