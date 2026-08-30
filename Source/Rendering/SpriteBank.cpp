#include "Rendering/SpriteBank.h"
#include "Core/Log.h"
#include <algorithm>

namespace ink {

void SpriteBank::Add(const std::string& name, int w, int h, const std::vector<uint32_t>& rgba) {
    SpriteFrame f;
    f.w = w;
    f.h = h;
    f.rgba = rgba;
    frames_[name] = std::move(f);
}

void SpriteBank::Add(const std::string& name, int w, int h, const uint32_t* rgba) {
    Add(name, w, h, std::vector<uint32_t>(rgba, rgba + w * h));
}

RectF SpriteBank::Rect(const std::string& name) const {
    auto it = entries_.find(name);
    if (it == entries_.end())
        return {};
    auto f = frames_.find(name);
    return {static_cast<double>(it->second.x), static_cast<double>(it->second.y),
            static_cast<double>(f->second.w), static_cast<double>(f->second.h)};
}

Vec2 SpriteBank::Size(const std::string& name) const {
    auto f = frames_.find(name);
    if (f == frames_.end())
        return {};
    return {f->second.w, f->second.h};
}

int SpriteBank::Build(const std::function<int(int, int, const uint32_t*)>& createTexture) {
    if (frames_.empty())
        return -1;
    // Shelf packing, 2px padding, capped at 1024 per side.
    const int maxSide = 1024;
    std::vector<std::string> names;
    for (const auto& [name, f] : frames_)
        names.push_back(name);
    std::sort(names.begin(), names.end(), [&](const std::string& a, const std::string& b) {
        return frames_[a].h > frames_[b].h;
    });

    struct Shelf {
        int xCursor = 2;
        int h = 0;
    };
    std::vector<Shelf> shelves;
    std::map<std::string, int> shelfOf;
    for (const std::string& name : names) {
        const SpriteFrame& f = frames_[name];
        int slot = -1;
        for (int i = 0; i < static_cast<int>(shelves.size()); ++i) {
            if (shelves[i].xCursor + f.w + 2 <= maxSide && shelves[i].h >= f.h) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            if (f.h + 2 > maxSide) {
                INK_LOG_ERROR("sprite {} taller than atlas", name);
                return -1;
            }
            shelves.push_back(Shelf{});
            slot = static_cast<int>(shelves.size()) - 1;
            shelves[slot].h = f.h;
        }
        shelfOf[name] = slot;
        // x assigned at layout below; remember width order
        shelves[slot].xCursor += f.w + 2;
        if (shelves[slot].h < f.h)
            shelves[slot].h = f.h;
    }
    // Assign y per shelf; compute atlas size.
    std::map<int, int> shelfY;
    int atlasH = 0;
    int atlasW = 0;
    for (std::size_t i = 0; i < shelves.size(); ++i) {
        shelfY[static_cast<int>(i)] = atlasH;
        atlasH += shelves[i].h + 2;
        int w = shelves[i].xCursor;
        atlasW = std::max(atlasW, w);
    }
    if (atlasW > maxSide || atlasH > maxSide) {
        INK_LOG_ERROR("sprite atlas overflow {}x{}", atlasW, atlasH);
        return -1;
    }
    std::vector<uint32_t> pixels(static_cast<std::size_t>(atlasW) * atlasH, 0);
    for (const std::string& name : names) {
        const SpriteFrame& f = frames_[name];
        int si = shelfOf[name];
        // recompute x by replaying widths of frames earlier on the same shelf
        int x = 2;
        for (const std::string& other : names) {
            if (other == name)
                break;
            if (shelfOf[other] == si)
                x += frames_[other].w + 2;
        }
        int y = shelfY[si];
        entries_[name] = Entry{x, y};
        for (int j = 0; j < f.h; ++j)
            for (int i = 0; i < f.w; ++i)
                pixels[static_cast<std::size_t>(y + j) * atlasW + (x + i)] =
                    f.rgba[static_cast<std::size_t>(j) * f.w + i];
    }
    textureId_ = createTexture(atlasW, atlasH, pixels.data());
    atlasW_ = atlasW;
    atlasH_ = atlasH;
    if (textureId_ < 0)
        return -1;
    INK_LOG_INFO("sprite atlas: {} frames -> {}x{}", static_cast<long>(frames_.size()), atlasW,
                 atlasH);
    return textureId_;
}


} // namespace ink
