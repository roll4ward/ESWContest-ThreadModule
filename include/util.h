#ifndef ROLL4_UTIL_H
#define ROLL4_UTIL_H

#define EXPECT_OR_EXIT(condition)   do {\
                                        if (!(condition)) goto exit;\
                                    } while (false)

#define EXPECT_NO_ERROR_OR_EXIT(statement) do {\
                                                if (statement) goto exit;\
                                            } while(false)

#define EXPECT_OR_DO(condition, statement)  do {\
                                                if (!(condition)) statement\
                                            } while(false)

#endif