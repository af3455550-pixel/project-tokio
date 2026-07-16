#include "Dialogue/Dialogue.h"
#include "Core/Json.h"

namespace ink {

bool DialogueBook::LoadJson(const std::string& json, std::string* err) {
    entries_.clear();
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("npcs");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "dialogue.json: expected array of npc dialogue";
        return false;
    }
    for (const auto& jn : arr->Items()) {
        std::string id = jn.Find("id") ? jn.Find("id")->AsString() : "npc";
        std::vector<DialogueLine> lines;
        const Json* ls = jn.Find("lines");
        if (ls && ls->IsArray()) {
            for (const auto& jl : ls->Items()) {
                DialogueLine l;
                l.speaker = jl.Find("speaker") ? jl.Find("speaker")->AsString("...") : "...";
                l.text = jl.Find("text") ? jl.Find("text")->AsString("") : "";
                lines.push_back(l);
            }
        }
        entries_.push_back({id, std::move(lines)});
    }
    return !entries_.empty();
}

const std::vector<DialogueLine>* DialogueBook::Lines(const std::string& npcId) const {
    for (const auto& [id, lines] : entries_)
        if (id == npcId)
            return &lines;
    return nullptr;
}

} // namespace ink
