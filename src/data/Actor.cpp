#include "data/Actor.h"

#include <algorithm>

QDebug operator<<(QDebug dbg, const Actor& actor)
{
    QString nl = "\n";
    QString out;
    out.append("Actor").append(nl);
    out.append(QStringLiteral("  Name:  ").append(actor.name).append(nl));
    out.append(QStringLiteral("  Role:  ").append(actor.role).append(nl));
    out.append(QStringLiteral("  Thumb: ").append(actor.thumb).append(nl));
    out.append(QStringLiteral("  ID:    ").append(actor.id).append(nl));
    out.append(QStringLiteral("  Order: ").append(QString::number(actor.order)).append(nl));
    dbg.nospace() << out;
    return dbg.maybeSpace();
}

void Actors::addActor(Actor actor)
{
    if (actor.order == 0 && !m_actors.empty()) {
        actor.order = m_actors.back()->order + 1;
    }

    // Try to merge into existing actor if possible (by id, otherwise by name).
    Actor* existing = nullptr;
    int idx = -1;

    if (!actor.id.isEmpty()) {
        for (int i = 0; i < m_actors.size(); ++i) {
            if (m_actors[i]->id == actor.id) {
                existing = m_actors[i];
                idx = i;
                break;
            }
        }
    }

    if (!existing) {
        for (int i = 0; i < m_actors.size(); ++i) {
            if (m_actors[i]->name.compare(actor.name, Qt::CaseInsensitive) == 0) {
                existing = m_actors[i];
                idx = i;
                break;
            }
        }
    }

    if (existing) {
        // Update fields (keep existing preserveImage if already set)
        existing->name = actor.name;
        existing->role = actor.role;
        existing->thumb = actor.thumb;
        if (actor.order != 0) existing->order = actor.order;
        existing->preserveImage = existing->preserveImage || actor.preserveImage;

        // Only overwrite image if incoming has one and existing doesn't request preservation
        if (!actor.image.isEmpty() && !existing->preserveImage) {
            existing->image = actor.image;
            existing->imageHasChanged = true;
        }
    } else {
        auto* a = new Actor(actor);
        m_actors.push_back(a);
    }
}

void Actors::removeActor(Actor* actor)
{
    // Note on clang-tidy: We can't use `auto*` for Qt6
    auto i = std::find(m_actors.begin(), m_actors.end(), actor);
    if (i != m_actors.end()) {
        m_actors.erase(i);
        delete actor;
    }
}

bool Actors::hasActors() const
{
    return !m_actors.isEmpty();
}

void Actors::clearImages()
{
    for (auto& actor : m_actors) {
        if (!actor->preserveImage) {
            actor->image = QByteArray();
        }
    }
}

Actors::~Actors()
{
    qDeleteAll(m_actors);
    m_actors.clear();
}

void Actors::setActors(QVector<Actor> actors)
{
    // Merge semantics:
    // - For each incoming actor, try to match existing by id, otherwise by name (case-insensitive).
    // - If matched: update fields, but do NOT overwrite existing image if existing->preserveImage is true.
    // - If not matched: create a new Actor and add it.
    // - Actors not present in the incoming list will be deleted (behavior preserves original replace semantics).

    QVector<Actor*> newActors;
    newActors.reserve(actors.size());

    QVector<char> used;
    used.resize(m_actors.size());
    std::fill(used.begin(), used.end(), 0);

    for (const Actor& a : actors) {
        Actor* existing = nullptr;
        int idx = -1;

        if (!a.id.isEmpty()) {
            for (int i = 0; i < m_actors.size(); ++i) {
                if (!used[i] && m_actors[i]->id == a.id) {
                    existing = m_actors[i];
                    idx = i;
                    break;
                }
            }
        }

        if (!existing) {
            for (int i = 0; i < m_actors.size(); ++i) {
                if (!used[i] && m_actors[i]->name.compare(a.name, Qt::CaseInsensitive) == 0) {
                    existing = m_actors[i];
                    idx = i;
                    break;
                }
            }
        }

        if (existing) {
            // Update fields, but respect preserveImage
            existing->name = a.name;
            existing->role = a.role;
            existing->thumb = a.thumb;
            if (a.order != 0) existing->order = a.order;
            existing->preserveImage = existing->preserveImage || a.preserveImage;

            if (!a.image.isEmpty() && !existing->preserveImage) {
                existing->image = a.image;
                existing->imageHasChanged = true;
            }

            newActors.push_back(existing);
            if (idx != -1) used[idx] = 1;
        } else {
            auto* actor = new Actor(a);
            newActors.push_back(actor);
        }
    }

    // Delete any old actors that were not reused
    for (int i = 0; i < m_actors.size(); ++i) {
        if (!used[i]) {
            delete m_actors[i];
        }
    }

    m_actors = std::move(newActors);
}

void Actors::removeAll()
{
    qDeleteAll(m_actors);
    m_actors.clear();
}

const QVector<Actor*>& Actors::actors()
{
    return m_actors;
}

QVector<const Actor*> Actors::actors() const
{
    QVector<Actor const*> actors;
    for (Actor const* a : m_actors) {
        actors << a;
    }
    return actors;
}
