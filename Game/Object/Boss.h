#pragma once
#include "Object/Character.h"


class Boss final:
    public Character{
public:
    Boss() = default;
    ~Boss()override;

    void Initialize(std::vector<Object3d*>models);

    void Update()override;

    void Draw()override;
    
};

