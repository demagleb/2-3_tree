#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

/**
 *  A sorted associative container made up of unique keys, which can be
 *  retrieved in logarithmic time. It's an implementation of 2-3 tree
 *
 *  @tparam T  Type of key objects.
 *
 */

template<class T>
class Set {
  private:
    static const size_t MAX_SONS = 4;

    struct Node {
        Node() = default;

        explicit Node(const T& val) : val(new T(val)) {}

        T* val = nullptr;
        Node* parent = nullptr;
        std::array<Node*, MAX_SONS> sons;
        size_t sons_size = 0;
    };

  public:
    class Iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
      private:
        // Check if Iterator is invalid O(log(n))
        void check_version_() const;

        const Node* cur_ = nullptr;
        const Set<T>* s_ = nullptr;
        uint64_t version_ = 0;

      public:
        Iterator(const Node* node, const Set<T>* s);

        Iterator() = default;

        Iterator& operator++();

        Iterator operator++(int);

        Iterator& operator--();

        Iterator operator--(int);

        T* operator->();

        bool operator!=(const Iterator& iter) const;

        bool operator==(const Iterator& iter) const;

        const T& operator*() const;
    };

    using iterator = Iterator;

  public:
    Set() = default;

    template<typename InputIterator>
    Set(InputIterator first, InputIterator last);

    Set(std::initializer_list<T> elems);

    Set(const Set<T>& s);

    Set(Set<T>&& s) noexcept;

    ~Set();

    Set<T>& operator=(const Set<T>& s);

    Set<T>& operator=(Set<T>&& s) noexcept;

    // Return number of elements O(1)
    inline size_t size() const { return size_; }

    // Checks whether the container is empty O(1)
    inline bool empty() const { return size_ == 0; }

    // Inserts element into the set, if the set doesn't already contain an
    // element with an equivalent key. O(log(n))
    void insert(const T& elem);

    // Removes elem from the set, if the set contain it O(log(n))
    void erase(const T& elem);

    // Returns an iterator to the beginning O(1)
    Iterator begin() const;

    // Returns an iterator to the end O(1)
    Iterator end() const;

    // Returns an iterator to the first element not less than the given key
    // O(log(n))
    Iterator lower_bound(const T& elem) const;

    // Returns an iterator to the element equal to the given key O(log(n))
    Iterator find(const T& elem) const;

  private:
    // Make node's data valid
    void update_(Node* node);

    // Processes the case if node has 4 sons O(log(n))
    void fix4sons_(Node* node);

    // Processes the case if node has 1 son O(log(n))
    void fix1sons_(Node* node);

    // Fix sons order in node O(1)
    void sort_sons(Node* node);

    // Returns pointer to the first node with element not less than the given key
    // O(log(n))
    Node* lower_bound_(const T& elem);


    // Returns pointer to the next node O(log(n))
    const Node* next_node_(const Node* cur_) const;

    // Returns pointer to the previous node O(log(n))
    const Node* prev_node_(const Node* cur_) const;

    // Copy tree
    Node* copy_(const Node* root);

    // Deletes all nodes of tree
    void destruct_(Node* root);

  private:
    Node* root_ = nullptr;
    size_t size_ = 0;
    uint64_t version_ = 0;
    Node END_NODE_;
};

template<class T>
template<class InputIterator>
Set<T>::Set(InputIterator first, InputIterator last) {
    for (InputIterator i = first; i != last; ++i) {
        insert(*i);
    }
}

template<class T>
Set<T>::Set(std::initializer_list<T> elems) {
    for (const auto& elem : elems) {
        insert(elem);
    }
}

template<class T>
typename Set<T>::Node* Set<T>::lower_bound_(const T& elem) {
    Node* node = root_;
    while (node->sons_size) {
        if (!(*(node->sons[0]->val) < elem)) {
            node = node->sons[0];
        } else if (!(*(node->sons[1]->val) < elem) || node->sons_size == 2) {
            node = node->sons[1];
        } else {
            node = node->sons[2];
        }
    }
    return node;
}

template<class T>
typename Set<T>::Iterator Set<T>::lower_bound(const T& elem) const {
    if (size_ == 0) {
        return end();
    }
    const Node* node = root_;
    while (node->sons_size) {
        if (!(*(node->sons[0]->val) < elem)) {
            node = node->sons[0];
        } else if (!(*(node->sons[1]->val) < elem)) {
            node = node->sons[1];
        } else if (node->sons_size == 2 || *node->sons[2]->val < elem) {
            return end();
        } else {
            node = node->sons[2];
        }
    }
    if (*node->val < elem) {
        return end();
    }
    return Iterator(node, this);
}

template<class T>
void Set<T>::fix4sons_(Set::Node* node) {
    if (node->sons_size != 4) return;
    Node* node2 = new Node();
    node2->sons[0] = node->sons[2];
    node2->sons[1] = node->sons[3];
    node2->sons_size = 2;
    node->sons_size = 2;
    update_(node2);
    update_(node);
    if (node == root_) {
        root_ = new Node();
        root_->sons[root_->sons_size++] = node;
        root_->sons[root_->sons_size++] = node2;
        update_(root_);
        return;
    }
    node->parent->sons[node->parent->sons_size++] = node2;
    update_(node->parent);
    fix4sons_(node->parent);
}

template<class T>
void Set<T>::update_(Set::Node* node) {
    if (node == nullptr) {
        return;
    }
    sort_sons(node);
    for (size_t i = 0; i < node->sons_size; ++i) {
        node->sons[i]->parent = node;
    }
    node->val = node->sons[node->sons_size - 1]->val;
}

template<class T>
void Set<T>::fix1sons_(Set::Node* node) {
    if (node == nullptr) {
        return;
    }
    if (node->sons_size != 1) {
        update_(node);
        return fix1sons_(node->parent);
    }
    if (node == root_) {
        root_ = node->sons[0];
        root_->parent = nullptr;
        delete node;
        return;
    }
    Node* bro = (node == node->parent->sons[1] ? node->parent->sons[0]
                                               : node->parent->sons[1]);
    bro->sons[bro->sons_size++] = node->sons[0];
    size_t pos =
        std::find(node->parent->sons.begin(),
                  node->parent->sons.begin() + node->parent->sons_size, node) -
        node->parent->sons.begin();
    while (pos != node->parent->sons_size - 1) {
        std::swap(node->parent->sons[pos], node->parent->sons[pos + 1]);
        pos++;
    }
    node->parent->sons_size--;
    delete node;
    update_(bro);
    fix4sons_(bro);
    update_(bro->parent);
    fix1sons_(bro->parent);
}

template<class T>
void Set<T>::sort_sons(Set::Node* node) {
    for (size_t i = node->sons_size - 2; i < MAX_SONS; --i) {
        if (*node->sons[i + 1]->val < *node->sons[i]->val) {
            std::swap(node->sons[i], node->sons[i + 1]);
        }
    }
}

template<class T>
typename Set<T>::Iterator Set<T>::find(const T& elem) const {
    Iterator iter = lower_bound(elem);
    if (iter == end()) {
        return end();
    }
    if (*iter < elem || elem < *iter) {
        return end();
    }
    return iter;
}

template<class T>
typename Set<T>::Iterator Set<T>::begin() const {
    if (size_ == 0) {
        return end();
    }
    const Node* node = root_;
    while (node->sons_size) {
        node = node->sons[0];
    }
    return Iterator(node, this);
}

template<class T>
typename Set<T>::Iterator Set<T>::end() const {
    return Iterator(&END_NODE_, this);
}

template<class T>
void Set<T>::insert(const T& elem) {
    if (find(elem) != end()) {
        return;
    }
    ++version_;
    ++size_;
    Node* node = new Node(elem);
    if (root_ == nullptr) {
        root_ = node;
        return;
    }
    Node* pos = lower_bound_(elem);
    if (pos->parent == nullptr) {
        root_ = new Node();
        root_->sons[root_->sons_size++] = pos;
        root_->sons[root_->sons_size++] = node;
        update_(root_);
        return;
    }
    pos->parent->sons[pos->parent->sons_size++] = node;
    update_(pos->parent);
    fix4sons_(pos->parent);
    while (pos->parent != nullptr) {
        pos = pos->parent;
        update_(pos->parent);
    }
}

template<class T>
void Set<T>::erase(const T& elem) {
    if (size_ == 0) return;

    Node* node = lower_bound_(elem);
    if (*node->val < elem || elem < *node->val) return;
    ++version_;
    --size_;
    if (node->parent == nullptr) {
        delete node->val;
        delete node;
        root_ = nullptr;
        return;
    }
    Node* parent = node->parent;
    size_t pos = std::find(parent->sons.begin(),
                           parent->sons.begin() + parent->sons_size, node) -
                 parent->sons.begin();
    while (pos != parent->sons_size - 1) {
        std::swap(parent->sons[pos], parent->sons[pos + 1]);
        pos++;
    }
    parent->sons_size--;

    delete node->val;
    delete node;
    update_(parent);
    fix1sons_(parent);
}

template<class T>
const typename Set<T>::Node* Set<T>::next_node_(const Node* cur_) const {
    const Node* son = cur_;
    const Node* par = cur_->parent;
    while (par != nullptr && son == par->sons[par->sons_size - 1]) {
        par = par->parent;
        son = son->parent;
    }
    if (par == nullptr) return &END_NODE_;
    son = *(
        std::find(par->sons.begin(), par->sons.begin() + par->sons_size, son) +
        1);
    while (son->sons_size) {
        son = son->sons[0];
    }
    if (son->val == nullptr) {
        throw std::exception();
    }
    return son;
}

template<class T>
const typename Set<T>::Node* Set<T>::prev_node_(const Node* cur_) const {
    if (cur_ == &END_NODE_) {
        cur_ = root_;
        while (cur_->sons_size) {
            cur_ = cur_->sons[cur_->sons_size - 1];
        }
        return cur_;
    }
    const Node* son = cur_;
    const Node* par = cur_->parent;
    while (par != nullptr && son == par->sons[0]) {
        par = par->parent;
        son = son->parent;
    }
    if (par == nullptr) return &END_NODE_;
    son = *(
        std::find(par->sons.begin(), par->sons.begin() + par->sons_size, son) -
        1);
    while (son->sons_size) {
        son = son->sons[son->sons_size - 1];
    }
    return son;
}

template<class T>
typename Set<T>::Node* Set<T>::copy_(const Node* root) {
    if (root == nullptr) {
        return nullptr;
    }
    Node* new_root = new Node();
    if (!root->sons_size) {
        new_root->val = new T(*root->val);
    }
    for (size_t i = 0; i < root->sons_size; ++i) {
        new_root->sons[i] = copy_(root->sons[i]);
        new_root->sons[i]->parent = new_root;
    }
    update_(new_root);
    return new_root;
}

template<class T>
Set<T>::Set(const Set<T>& s) {
    root_ = nullptr;
    size_ = 0;
    for (const auto& item : s) {
        insert(item);
    }
}

template<class T>
Set<T>& Set<T>::operator=(const Set<T>& s) {
    if (this == &s) {
        return *this;
    }
    destruct_(root_);
    root_ = nullptr;
    size_ = 0;
    for (const auto& item : s) {
        insert(item);
    }
    version_++;
    return *this;
}

template<class T>
Set<T>::~Set() {
    destruct_(root_);
}

template<class T>
void Set<T>::destruct_(Set::Node* root) {
    if (root == nullptr) {
        return;
    }
    for (size_t i = 0; i < root->sons_size; ++i) {
        destruct_(root->sons[i]);
    }
    if (root->sons_size == 0) {
        delete root->val;
    }
    delete root;
}

template<class T>
Set<T>::Set(Set<T>&& s) noexcept {
    std::swap(s.root_, root_);
    std::swap(s.size_, size_);
    std::swap(s.version_, version_);
    std::swap(s.END_NODE_, END_NODE_);
}

template<class T>
Set<T>& Set<T>::operator=(Set<T>&& s) noexcept {
    if (this == &s) {
        return *this;
    }
    std::swap(s.root_, root_);
    std::swap(s.size_, size_);
    std::swap(s.version_, version_);
    std::swap(s.END_NODE_, END_NODE_);
    return *this;
}

template<class T>
Set<T>::Iterator::Iterator(const Set<T>::Node* node, const Set<T>* s)
    : cur_(node), s_(s), version_(s->version_) {}

template<class T>
void Set<T>::Iterator::check_version_() const {
    if (version_ != s_->version_) {
        throw std::out_of_range("invalid iterator");
    }
}

template<class T>
typename Set<T>::Iterator& Set<T>::Iterator::operator++() {
    check_version_();
    cur_ = s_->next_node_(cur_);
    return *this;
}

template<class T>
typename Set<T>::Iterator& Set<T>::Iterator::operator--() {
    check_version_();
    cur_ = s_->prev_node_(cur_);
    return *this;
}

template<class T>
bool Set<T>::Iterator::operator!=(const typename Set<T>::Iterator& iter) const {
    check_version_();
    return s_ != iter.s_ || cur_ != iter.cur_;
}

template<class T>
const T& Set<T>::Iterator::operator*() const {
    check_version_();
    return *cur_->val;
}

template<class T>
bool Set<T>::Iterator::operator==(const Iterator& iter) const {
    check_version_();
    return !operator!=(iter);
}

template<class T>
T* Set<T>::Iterator::operator->() {
    check_version_();
    return cur_->val;
}

template<class T>
typename Set<T>::Iterator Set<T>::Iterator::operator++(int) {
    check_version_();
    Iterator copy = *this;
    this->operator++();
    return copy;
}

template<class T>
typename Set<T>::Iterator Set<T>::Iterator::operator--(int) {
    check_version_();
    Iterator copy = *this;
    this->operator--();
    return copy;
}
