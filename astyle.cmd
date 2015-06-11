
find  . -iname "*.[c|h]" | xargs -n1 -I "ZZZZZ" astyle --style=ansi --indent=spaces=8 --indent-switches --pad-oper --pad-header --add-brackets --suffix=none  "ZZZZZ"
