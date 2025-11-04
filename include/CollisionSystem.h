#pragma once
#include "Scene.h"
#include "AABBCollider.h"

class CollisionSystem {
public:
    static void Update(Scene& scene);
    
private:
    static void ResolveCollision(
        std::shared_ptr<Entity> a, std::shared_ptr<Entity> b,
        AABBCollider* colA, AABBCollider* colB);
};

