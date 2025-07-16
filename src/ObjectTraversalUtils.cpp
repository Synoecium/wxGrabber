#include "../include/ObjectTraversalUtils.h"
#include <algorithm>

namespace ObjectTraversalUtils {

bool FindAndRemoveObject(std::vector<std::shared_ptr<DataParser::DataObject>>& objects, 
                        std::shared_ptr<DataParser::DataObject> targetObj) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [targetObj](const std::shared_ptr<DataParser::DataObject> obj) {
            return obj == targetObj;
        });
    
    if (it != objects.end()) {
        objects.erase(it);
        return true;
    }
    return false;
}

std::shared_ptr<DataParser::DataObject> FindObjectByUID(const std::vector<std::shared_ptr<DataParser::DataObject>>& objects, 
                                                       uint32_t uid) {
    for (const auto& obj : objects) {
        if (obj->ui_id == uid) {
            return obj;
        }
        
        // Check nested objects
        if (obj->isNested()) {
            const auto& nested = obj->getNestedObjects();
            auto found = FindObjectByUID(nested, uid);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

bool FindAndReplaceObject(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                         std::shared_ptr<DataParser::DataObject> targetObj,
                         std::shared_ptr<DataParser::DataObject> replacementObj) {
    for (auto& obj : objects) {
        if (obj == targetObj) {
            obj = replacementObj;
            return true;
        }
        
        // Check nested objects
        if (obj->isNested()) {
            auto& nested = obj->getNestedObjects();
            if (FindAndReplaceObject(nested, targetObj, replacementObj)) {
                return true;
            }
        }
    }
    return false;
}

int CountObjects(const std::vector<std::shared_ptr<DataParser::DataObject>>& objects) {
    int count = 0;
    for (const auto& obj : objects) {
        count++;
        if (obj->isNested()) {
            const auto& nested = obj->getNestedObjects();
            count += CountObjects(nested);
        }
    }
    return count;
}

std::vector<std::shared_ptr<DataParser::DataObject>>* FindParentVector(
    std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
    std::shared_ptr<DataParser::DataObject> targetObj) {
    for (auto& obj : objects) {
        if (obj == targetObj) {
            return &objects;
        }
        
        if (obj->isNested()) {
            auto& nested = obj->getNestedObjects();
            auto* result = FindParentVector(nested, targetObj);
            if (result) return result;
        }
    }
    return nullptr;
}

bool InsertAfterTarget(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                      std::shared_ptr<DataParser::DataObject> toInsert,
                      std::shared_ptr<DataParser::DataObject> target) {
    for (size_t i = 0; i < objects.size(); ++i) {
        if (objects[i] == target) {
            objects.insert(objects.begin() + i + 1, toInsert);
            return true;
        }
        
        if (objects[i]->isNested()) {
            auto& nested = objects[i]->getNestedObjects();
            if (InsertAfterTarget(nested, toInsert, target)) {
                return true;
            }
        }
    }
    return false;
}

bool MoveObjectUp(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                 std::shared_ptr<DataParser::DataObject> targetObj) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [targetObj](const std::shared_ptr<DataParser::DataObject> obj) {
            return obj == targetObj;
        });

    if (it == objects.begin()) {
        return false; // Already at the top or not found
    }

    // Move the object up by swapping with the previous one
    std::iter_swap(it, std::prev(it));
    return true;
}

bool MoveObjectDown(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                   std::shared_ptr<DataParser::DataObject> targetObj) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [targetObj](const std::shared_ptr<DataParser::DataObject> obj) {
            return obj == targetObj;
        });

    if (it == objects.end() || std::next(it) == objects.end()) {
        return false; // Already at the bottom or not found
    }

    // Move the object down by swapping with the next one
    std::iter_swap(it, std::next(it));
    return true;
}

} // namespace ObjectTraversalUtils 