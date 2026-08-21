#include "routing_tree.h"


namespace http {

std::vector<std::string> parse(std::string path) 
{   
    size_t len = path.length();
    std::vector<std::string> path_elems;
    switch(len) {
        case 0: return {};
        case 1: 
        {
            if (path[0] == '/') return {"/"};
            return {};
        }
        default: 
        {
            if (path[0] != '/') return {};
            if (path.back() == '/') path.pop_back();
            
            size_t fst = path.find('/');
            size_t sec = path.find('/', fst+1); 
            while (sec != std::string::npos) {
                path_elems.push_back(path.substr(fst+1, sec-fst-1));
                fst = sec; 
                sec = path.find('/', sec+1);    
            }
            path_elems.push_back(path.substr(fst+1));
        }
    }
    return path_elems;
}


RoutingTree::RNode::RNode(
    std::string id, 
    std::optional<Handler>&& h
) : id(std::move(id)), h(std::move(h)) {}


std::optional<Handler> RoutingTree::get(const Method& mtd, const std::string& path) const noexcept
{
    auto root_it = root_.find(mtd);
    if (root_it == root_.end()) {
        return std::nullopt;
    }

    const Childs* chs = &root_it->second;

    std::vector<std::string> path_elems = parse(path);
    if (path_elems.empty()) return std::nullopt;

    const std::string& target = path_elems.back();

    for (auto&& elem : path_elems) {
        bool found = false;
        for (auto&& child : *chs) {
            if (child->id == elem) {
                if (elem == target) {
                    return child->h;
                }
                chs = &child->chs; 
                found = true;
                break;
            }
        }
        if (!found) break;
    }
    return std::nullopt;
}


bool RoutingTree::add(const Method& mtd, const std::string& path, Handler& h) noexcept
{
    if (mtd == Method::UNKNOWN) return false;
    std::vector<std::string> path_elems = parse(path);
    return add(mtd, std::move(path_elems), h);
}


bool RoutingTree::add(const Method& mtd, std::vector<std::string>&& path_elems, Handler h) noexcept
{
    if (path_elems.empty() || mtd == Method::UNKNOWN) return false;

    Childs* chs = &root_[mtd];
    size_t elem_size = path_elems.size();

    for (size_t i = 0; i < elem_size; ++i) {
        const std::string& value = path_elems[i];
        const bool is_last = (i + 1 == elem_size);

        bool found = false;
        for (auto&& child : *chs) {
            if (child->id == value) {
                if (is_last) {
                    if (child->h != std::nullopt) return false;
                    child->h = std::move(h);
                }
                chs = &child->chs;
                found = true;
                break;
            }
        }
        if (found) continue;

        std::shared_ptr<RNode> new_child = is_last
            ? std::make_shared<RNode>(value, std::move(h))
            : std::make_shared<RNode>(value);

        chs->push_back(std::move(new_child));
        chs = &chs->back()->chs;
    }
    ++size_;
    return true;
}


size_t RoutingTree::size() const noexcept 
{ 
    return size_;
}


bool RoutingTree::empty() const noexcept
{
    if (!size_) return true;
    return false;
}

} // namespace http