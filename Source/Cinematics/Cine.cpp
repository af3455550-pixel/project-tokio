#include "Cinematics/Cine.h"

namespace ink {

CineDef Cine::TextCards(const std::string& id, std::vector<std::string> texts, double durPerCard) {
    CineDef def;
    def.id = id;
    double t = 0.0;
    for (auto& txt : texts) {
        CineCard c;
        c.at = t;
        c.dur = durPerCard;
        c.text = std::move(txt);
        c.letterbox = true;
        def.cards.push_back(c);
        t += durPerCard;
    }
    def.total = t;
    return def;
}

} // namespace ink
