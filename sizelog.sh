counter=0
diskSpace=0

RED='\e[31m'
GREEN='\e[32m'
YELLOW='\e[33m'
BLUE='\e[34m'
RESET='\e[0m'

BOLD='\e[1m'
 

countSize(){
    subcounter=0
    echo -e $RED"CARTELLA: $1 $RESET"
    for file in $1/*; do
        name=$(basename $file)
        echo -n "$name -> "
        
         
        size=$(stat --printf "%s" $file)
        subcounter=$(($subcounter+$size))
        echo -e "$YELLOW"$size"B"$RESET #In byte
        
        
     
    done
    echo -e $BLUE"DIMENSIONE CARTELLA: $subcounter""B""$RESET"
    echo -e "----------------"
    
    counter=$(($subcounter + $counter))
    if [ "$1" = "bin" ]; then
        diskSpace=$subcounter
    fi 
}

countRecursive(){
    for dir in $1/*; do
        if [ -d $dir ]; then
            countSize $dir
        fi
    done
}



countRecursive C/src/kernel
countRecursive ASM
countSize elf
countRecursive obj
countSize bin
countSize lib

echo -e $RED$BOLD"DIMENSIONE TOTALE (IN REPO) => $counter""B"$RESET$RESET
echo -e $GREEN$BOLD"SPAZIO OCCUPATO SUL DISCO => $diskSpace""B\n"$RESET$RESET

 