import { useDispatch } from "react-redux";
import type { AppDispatch } from "../store";

// Hook для типизированного dispatch
export const useAppDispatch = () => useDispatch<AppDispatch>();
