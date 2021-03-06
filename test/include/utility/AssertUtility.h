#ifndef LIGHTDB_ASSERTUTILITY_H
#define LIGHTDB_ASSERTUTILITY_H

#define ASSERT_TYPE(instance, type) \
  ASSERT_NE(dynamic_cast<const type*>(&(instance)), nullptr)

#define EXPECT_TYPE(instance, type) \
  EXPECT_NE(dynamic_cast<type*>(&(instance)), nullptr)

#endif //LIGHTDB_ASSERTUTILITY_H
