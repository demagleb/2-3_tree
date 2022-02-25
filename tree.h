#include <vector>
#include <iostream>
#include <algorithm>
#include <exception>
#include <stdexcept>


template<class T>
struct Node_ {
    Node_() = default;

    explicit Node_(const T &val) : val(new T(val)) {}

    T *val = nullptr;
    Node_ *parent = nullptr;
    Node_ *sons[4];
    int sons_size = 0;
};

template<class T>
class Set;

template<class T>
class Iterator : public std::iterator<std::bidirectional_iterator_tag, T> {

    friend Set<T>;
    using Node = Node_<T>;
    using Set = Set<T>;

private:
    Iterator(const Node *node, const Set *s);

    void check_version_() const;

    const Node *cur_ = nullptr;
    const Set *s_ = nullptr;
    uint64_t version_ = 0;
public:
    Iterator() = default;

    Iterator &operator++();

    Iterator<T> operator++(int);

    Iterator &operator--();

    Iterator<T> operator--(int);

    T *operator->();

    bool operator!=(const Iterator &iter) const;

    bool operator==(const Iterator &iter) const;

    const T &operator*() const;
};

template<class T>
class Set {
public:
    friend Iterator<T>;

    using iterator = Iterator<T>;
    using Node = Node_<T>;

    Set() = default;

    template<typename Iterator>
    Set(Iterator first, Iterator last);

    Set(std::initializer_list<T> elems);

    Set(const Set<T> &s);

    Set(Set<T> &&s) noexcept;

    ~Set();

    Set<T> &operator=(const Set<T> &s);

    Set<T> &operator=(Set<T> &&s) noexcept;

    size_t size() const;

    bool empty() const;

    void insert(const T &elem);

    void erase(const T &elem);

    iterator begin() const;

    iterator end() const;

    iterator lower_bound(const T &elem) const;

    iterator find(const T &elem) const;

private:
    void update_(Node *node);

    void fix4sons_(Node *node);

    void fix1sons_(Node *node);

    void sort_sons(Node *node);

    Node *lower_bound_(const T &elem);

    const Node *next_node_(const Node *cur_) const;

    const Node *prev_node_(const Node *cur_) const;


    Node *copy_(const Node *root);

    void destruct_(Node *root);

private:
    Node *root_ = nullptr;
    size_t size_ = 0;
    uint64_t version_ = 0;
    Node END_NODE_;
};

template<class T>
size_t Set<T>::size() const {
    return size_;
}

template<class T>
template<typename Iterator>
Set<T>::Set(Iterator first, Iterator last) {
    for (auto i = first; i != last; ++i) {
        insert(*i);
    }
}

template<class T>
Set<T>::Set(std::initializer_list<T> elems) {
    for (const auto &elem: elems) {
        insert(elem);
    }
}

template<class T>
bool Set<T>::empty() const {
    return size_ == 0;
}

template<class T>
typename Set<T>::Node *Set<T>::lower_bound_(const T &elem) {
    Node *node = root_;
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
typename Set<T>::iterator Set<T>::lower_bound(const T &elem) const {
    if (size_ == 0) {
        return end();
    }
    const Node *node = root_;
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
    return iterator(node, this);
}

template<class T>
void Set<T>::fix4sons_(Set::Node *node) {
    if (node->sons_size != 4)
        return;
    Node *node2 = new Node();
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
void Set<T>::update_(Set::Node *node) {
    if (node == nullptr) {
        return;
    }
    sort_sons(node);
    for (int i = 0; i < node->sons_size; ++i) {
        node->sons[i]->parent = node;
    }
    node->val = node->sons[node->sons_size - 1]->val;
}

template<class T>
void Set<T>::fix1sons_(Set::Node *node) {
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
    Node *bro = (node == node->parent->sons[1] ? node->parent->sons[0] : node->parent->sons[1]);
    bro->sons[bro->sons_size++] = node->sons[0];
    int pos = std::find(node->parent->sons, node->parent->sons + node->parent->sons_size, node) - node->parent->sons;
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
void Set<T>::sort_sons(Set::Node *node) {
    for (int i = node->sons_size - 2; i >= 0; --i) {
        if (*node->sons[i + 1]->val < *node->sons[i]->val) {
            std::swap(node->sons[i], node->sons[i + 1]);
        }
    }
}

template<class T>
typename Set<T>::iterator Set<T>::find(const T &elem) const {
    auto iter = lower_bound(elem);
    if (iter == end()) {
        return end();
    }
    if (*iter < elem || elem < *iter) {
        return end();
    }
    return iter;
}

template<class T>
typename Set<T>::iterator Set<T>::begin() const {
    if (size_ == 0) {
        return end();
    }
    const Node *node = root_;
    while (node->sons_size) {
        node = node->sons[0];
    }
    return iterator(node, this);
}

template<class T>
typename Set<T>::iterator Set<T>::end() const {
    return iterator(&END_NODE_, this);
}

template<class T>
void Set<T>::insert(const T &elem) {
    if (find(elem) != end()) {
        return;
    }
    ++version_;
    ++size_;
    Node *node = new Node(elem);
    if (root_ == nullptr) {
        root_ = node;
        return;
    }
    Node *pos = lower_bound_(elem);
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
void Set<T>::erase(const T &elem) {
    if (size_ == 0)
        return;

    Node *node = lower_bound_(elem);
    if (*node->val < elem || elem < *node->val)
        return;
    ++version_;
    --size_;
    if (node->parent == nullptr) {
        delete node->val;
        delete node;
        root_ = nullptr;
        return;
    }
    Node *parent = node->parent;
    int pos = std::find(parent->sons, parent->sons + parent->sons_size, node) - parent->sons;
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
const typename Set<T>::Node *Set<T>::next_node_(const Node *cur_) const {
    auto son = cur_;
    auto par = cur_->parent;
    while (par != nullptr && son == par->sons[par->sons_size - 1]) {
        par = par->parent;
        son = son->parent;
    }
    if (par == nullptr)
        return &END_NODE_;
    son = *(std::find(par->sons, par->sons + par->sons_size, son) + 1);
    while (son->sons_size) {
        son = son->sons[0];
    }
    if (son->val == nullptr) {
        throw std::exception();
    }
    return son;
}

template<class T>
const typename Set<T>::Node *Set<T>::prev_node_(const Node *cur_) const {
    if (cur_ == &END_NODE_) {
        cur_ = root_;
        while (cur_->sons_size) {
            cur_ = cur_->sons[cur_->sons_size - 1];
        }
        return cur_;
    }
    auto son = cur_;
    auto par = cur_->parent;
    while (par != nullptr && son == par->sons[0]) {
        par = par->parent;
        son = son->parent;
    }
    if (par == nullptr)
        return &END_NODE_;
    son = *(std::find(par->sons, par->sons + par->sons_size, son) - 1);
    while (son->sons_size) {
        son = son->sons[son->sons_size - 1];
    }
    return son;
}

template<class T>
typename Set<T>::Node *Set<T>::copy_(const Node *root) {
    if (root == nullptr) {
        return nullptr;
    }
    Node *new_root = new Node();
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
Set<T>::Set(const Set<T> &s) {
    root_ = nullptr;
    size_ = 0;
    for (const auto &item: s) {
        insert(item);
    }
}

template<class T>
Set<T> &Set<T>::operator=(const Set<T> &s) {
    if (this == &s) {
        return *this;
    }
    destruct_(root_);
    root_ = nullptr;
    size_ = 0;
    for (const auto &item: s) {
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
void Set<T>::destruct_(Set::Node *root) {
    if (root == nullptr) {
        return;
    }
    for (int i = 0; i < root->sons_size; ++i) {
        destruct_(root->sons[i]);
    }
    if (root->sons_size == 0) {
        delete root->val;
    }
    delete root;
}

template<class T>
Set<T>::Set(Set<T> &&s) noexcept {
    std::swap(s.root_, root_);
    std::swap(s.size_, size_);
    std::swap(s.version_, version_);
    std::swap(s.END_NODE_, END_NODE_);
}

template<class T>
Set<T> &Set<T>::operator=(Set<T> &&s) noexcept {
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
Iterator<T>::Iterator(const Iterator::Node *node, const Set *s) : cur_(node), s_(s), version_(s->version_) {}

template<class T>
void Iterator<T>::check_version_() const {
    if (version_ != s_->version_) {
        throw std::out_of_range("invalid iterator");
    }
}

template<class T>
Iterator<T> &Iterator<T>::operator++() {
    check_version_();
    cur_ = s_->next_node_(cur_);
    return *this;
}

template<class T>
Iterator<T> &Iterator<T>::operator--() {
    check_version_();
    cur_ = s_->prev_node_(cur_);
    return *this;
}

template<class T>
bool Iterator<T>::operator!=(const Iterator &iter) const {
    check_version_();
    return s_ != iter.s_ || cur_ != iter.cur_;
}

template<class T>
const T &Iterator<T>::operator*() const {
    check_version_();
    auto x = cur_->val;
    return *x;
}

template<class T>
bool Iterator<T>::operator==(const Iterator &iter) const {
    check_version_();
    return !operator!=(iter);
}

template<class T>
T *Iterator<T>::operator->() {
    check_version_();
    return cur_->val;
}

template<class T>
Iterator<T> Iterator<T>::operator++(int) {
    check_version_();
    Iterator<T> copy = *this;
    this->operator++();
    return copy;
}

template<class T>
Iterator<T> Iterator<T>::operator--(int) {
    check_version_();
    Iterator<T> copy = *this;
    this->operator--();
    return copy;
}