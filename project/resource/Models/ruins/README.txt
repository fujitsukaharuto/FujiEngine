RUINS シーン (resource/Json/Level/RUINS.json) のモデル

■ Poly Haven (https://polyhaven.com) のモデル  ライセンス: CC0 (パブリックドメイン相当・商用可・表記不要)
  元のアセット名 → このフォルダのファイル
  fern_02                    → fern_a〜d
  shrub_02 / shrub_03        → shrub2_a〜d / shrub3_a〜d
  weed_plant_02              → weed_a〜e
  nettle_plant               → nettle_tall / nettle_med / nettle_small
  dandelion_01               → dandelion_c / dandelion_d
  shrub_sorrel_01            → sorrel
  dry_branches_medium_01     → branch_a〜c
  rock_moss_set_01 / 02      → rockm1_1〜6 / rockm2_7〜13
  boulder_01                 → boulder (間引き 0.2)
  island_tree_02             → tree_small (葉を板に置き換え・枝と幹を間引き)
  jacaranda_tree             → tree_big   (同上)
  tree_stump_01 / dead_tree_trunk_02 → stump / dead_trunk (間引き)
  covered_car / concrete_road_barrier_02 / Barrel_01 / old_tyre / rusted_wheel_rim_01
  metal_trash_can (rust の方) / utility_box_02 / street_lamp_01 / concrete_cat_statue
  mounted_fluorescent_lights / security_light
                             → covered_car / barrier / barrel / tyre / wheel_rim / trash_can
                               / utility_box / street_lamp / cat_statue / fluo_a・fluo_d / security_light
  テクスチャは 1k (resource/Textures/ruins/)。葉は拡散色とアルファを合わせた *_diffa_1k.png

  加工: エンジンは glTF のノードの変換を読まないので、1 つの glTF に並んでいた種類を 1 ファイルずつに分け、
  変換を頂点へ焼いて原点へ寄せた。KHR_texture_transform は UV へ焼いた。ガラスは不透明の暗い色にした
  (物全体が半透明扱いになるため)。

■ 自作 (Blender のスクリプトで生成)
  bld_a〜f (崩れたビル) / hall (屋根の抜けたホール) / tower_a〜d (遠景の高層ビル) / overpass (落ちた高架)
  road / walk_l / walk_r / plaza / ground / grass_a〜c / rubble_a〜c / puddle_a・b / pool
  貼っているテクスチャは Poly Haven (CC0): concrete_layers_02 / concrete_moss / grey_plaster_02 / concrete_debris
  / asphalt_02 / cracked_concrete / overgrown_concrete_pavers / leafy_grass / aerial_grass_rock / broken_brick_wall
  草の葉の色 grass_blade.png は自作。
