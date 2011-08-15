(* 
#load "str.cma";;
#load "unix.cma";;
*)

(* woohoo! my own exception! *)
exception EntryException of string

(* read next line from channel returning status and line *)
let next_line chan =
  try
    (true, input_line chan)
  with End_of_file -> (false, "");;

(* simplified split function, assumes "," *)
let split = Str.split(Str.regexp_string ",");;

(* splits a string and checks for a match *)
let matches str column value =
  let comp = int_of_string (List.nth (split str) column) in
  match comp = value with 
  | true -> 1
  | false -> 0
  ;;

(* tail recursive, accumulates matches to produce a count *)
let rec count accumulator chan column value =
  let (status, line) = next_line chan in
  match status with
  | false -> accumulator
  | true -> count(accumulator + matches line column value) chan column value
  ;;

(* entry point to tail recursive count, starts at 0 *)
let count channel column value = 
  count 0 channel column value
  ;;

(* simple prompt/input pattern with rudimentary validation *)
let rec get prompt =
  try
    let () = print_string prompt in
    let n = read_int() in
    match n >= 0 with
    | true -> n
    | false -> raise (EntryException("Must be >= 0"))
  with 
  | Failure(s) ->
    Printf.printf "That isn't a number\nEnter an integer >= 0\n";
    get prompt
  | EntryException(s) ->
    Printf.printf "%s\nEnter an integer => 0\n" s;
    get prompt
  ;;

(* main()ish *)
let c = get "Column: " in
let v = get "Value: " in
let channel = open_in "data.txt" in
  Printf.printf "%f\n" (Unix.gettimeofday());
  Printf.printf "%d\n" (count channel c v);
  Printf.printf "%f\n" (Unix.gettimeofday());
  close_in channel
  ;;
  
