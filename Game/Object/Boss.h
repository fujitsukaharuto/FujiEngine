#pragma once
#include "Object/Character.h"


class Boss final:
    public Character{
public:
    Boss() = default;
    ~Boss()override;

    void Initialize(std::vector<Model*>models);

    void Update()override;

    void Draw()override;
    
};

