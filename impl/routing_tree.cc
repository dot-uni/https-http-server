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
    std::optional<Handler> h
) : id(std::move(id)), h(std::move(h)) {}


std::optional<Handler> RoutingTree::get(const Method& mtd, const std::string& path) const noexcept
{
    if (mtd == Method::UNKNOWN) return std::nullopt;

    auto root_it = root_.find(toString(mtd));
    if (root_it == root_.end()) return std::nullopt;

    const std::vector<std::unique_ptr<RNode>>* childs = &root_it->second;

    std::vector<std::string> path_elems = parse(path);
    if (path_elems.empty()) return std::nullopt;

    const std::string& target = path_elems.back();

    for (auto&& elem : path_elems) {
        bool found = false;
        for (auto&& child : *childs) {
            if (child->id == elem) {
                if (elem == target) {
                    return child->h;
                }
                childs = &child->childs; 
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
    return add(mtd, std::forward<std::vector<std::string>>(path_elems), h);
}


bool RoutingTree::add(const Method& mtd, std::vector<std::string>&& path_elems, Handler h) noexcept
{
    if (path_elems.empty() || mtd == Method::UNKNOWN) return false;

    std::vector<std::unique_ptr<RNode>>* childs = &root_[toString(mtd)];
    const size_t elem_size = path_elems.size();

    for (size_t i = 0; i < elem_size; ++i) {
        const std::string& value = path_elems[i];
        const bool is_last = (i + 1 == elem_size);

        bool found = false;
        for (auto&& child : *childs) {
            if (child->id == value) {
                if (is_last) {
                    if (child->h != std::nullopt) return false;
                    child->h = std::move(h);
                }
                childs = &child->childs;
                found = true;
                break;
            }
        }
        if (found) continue;

        std::unique_ptr<RNode> new_child = is_last
            ? std::make_unique<RNode>(value, std::move(h))
            : std::make_unique<RNode>(value);

        childs->push_back(std::move(new_child));
        childs = &childs->back()->childs;
    }
    return true;
}

} // namespace http