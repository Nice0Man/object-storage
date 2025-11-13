import { configureStore } from "@reduxjs/toolkit";
import authReducer from "./authSlice";
import bucketsReducer from "./bucketsSlice";
import objectsReducer from "./objectsSlice";
import usersReducer from "./usersSlice";
import statsReducer from "./statsSlice";

export const store = configureStore({
  reducer: {
    auth: authReducer,
    buckets: bucketsReducer,
    objects: objectsReducer,
    users: usersReducer,
    stats: statsReducer,
  },
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;

export default store;
