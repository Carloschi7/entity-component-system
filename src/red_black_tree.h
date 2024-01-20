#pragma once
#include <type_traits>
#include "types.h"


class BatchAllocator
{
	//TODO
};

//To replace with allocators
static void* allocate_node(u32 size)
{
	void* ptr = ::operator new(size);
	std::memset(ptr, 0, size);
	return ptr;
}

static void free_node(void* mem) 
{
	::operator delete(mem);
}

template<typename Type, typename Index = u64, class Allocator = std::allocator<Type>>
class RedBlackTree
{
	static_assert(std::is_integral_v<Index>, "Index needs to be of integral type");

	enum class Color { Black = 0, Red };
	struct TreeNode
	{
		Color color;
		Index index;
		Type* content;
		TreeNode* left, *right, *parent;
	};
public:
	RedBlackTree() : root{nullptr} {}

	template<class... Args>
	void emplace_node(Index value, Args&&... args)
	{
		static_assert(std::is_constructible_v<Type, Args...>, "The type needs to be constructible with the given params");

		if (root == nullptr) {
			root = static_cast<TreeNode*>(allocate_node(sizeof(TreeNode)));
			root->color = Color::Black;
			root->index = value;
			root->content = new Type{ std::forward<Args>(args)... };
			return;
		}

		TreeNode* node = nullptr;

		{
			TreeNode** _iter = &root;
			TreeNode* _parent = nullptr;
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

			*_iter = static_cast<TreeNode*>(allocate_node(sizeof(TreeNode)));
			(*_iter)->color = Color::Red;
			(*_iter)->parent = _parent;
			(*_iter)->index = value;
			(*_iter)->content = new Type{ std::forward<Args>(args)... }; 

			node = *_iter;
		}

		//No need to fixup
		if (node->parent->color == Color::Black)
			return;

		emplace_node_fixup(node);
	}

	TreeNode* get_node(Index index)
	{
		TreeNode* node = root;
		while (node == nullptr || node->index != index) {
			if (node->index < index)
				node = node->right;
			else
				node = node->left;
		}

		return node;
	}

	void delete_node(Index index)
	{
		TreeNode* node = root;
		while(node == nullptr || node->index != index){
			if(node->index < index)
				node = node->right;
			else
				node = node->left;
		}

		if(node == nullptr)
			return;

		isolate_node(node);
		delete_node_fixup(node);
		cleanup_terminal_node(node);
	}
private:
	void isolate_node(TreeNode*& node)
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
			//node has both children
			TreeNode* iter = node->left;
			while (iter->right)
				iter = iter->right;

			swap_node_data(node, iter);
			node = iter;
		}
	}

	void emplace_node_fixup(TreeNode* node)
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

	void delete_node_fixup(TreeNode* node)
	{
		if (node->parent == nullptr)
			return;

		TreeNode* parent = node->parent;
		TreeNode* sibling = sibling_node(node);
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

	void cleanup_terminal_node(TreeNode* node)
	{
		delete node->content;
		free_node(node);
	}

	void swap_node_data(TreeNode* first, TreeNode* second)
	{
		Type* tmp = first->content;
		first->content = second->content;
		second->content = tmp;

		Index index = first->index;
		first->index = second->index;
		second->index = index;
	}

	void rotate_left(TreeNode* node)
	{
		TreeNode* parent = node->parent;
		TreeNode* grandfather = grandfather_node(node);
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
		TreeNode* node_left_branch = node->left;
		node->left = parent;
		parent->parent = node;

		parent->right = node_left_branch;
		if(node_left_branch != nullptr) node_left_branch->parent = parent;
	}

	void rotate_right(TreeNode* node)
	{
		TreeNode* parent = node->parent;
		TreeNode* grandfather = grandfather_node(node);
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
		TreeNode* node_right_branch = node->right;
		node->right = parent;
		parent->parent = node;

		parent->left = node_right_branch;
		if (node_right_branch != nullptr) node_right_branch->parent = parent;
	}

	inline TreeNode* grandfather_node(TreeNode* node) 
	{
		assert(node->parent != nullptr);
		return node->parent->parent;
	}

	inline TreeNode* uncle_node(TreeNode* node)
	{
		TreeNode* gfather = grandfather_node(node);
		assert(gfather != nullptr);

		return node->parent == gfather->right ? gfather->left : gfather->right;
	}

	inline TreeNode* sibling_node(TreeNode* node)
	{
		assert(node->parent != nullptr);
		if(node == node->parent->right)
			return node->parent->left;
		else
			return node->parent->right;
	}
private:
	TreeNode* root;
};
