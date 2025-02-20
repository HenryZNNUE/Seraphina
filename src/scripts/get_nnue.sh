cyan='\e[1;96m'
blue='\e[1;94m'
red='\e[1;91m'
yellow='\e[1;93m'
default='\e[0m'

function get_nnue {
    nnue_file="$(grep -oP '#define Seraphina_NNUE "\K[^"]+' nnue.h)"
    echo -e "${cyan}Seraphina NNUE File: "$nnue_file
    url="https://raw.githubusercontent.com/HenryZNNUE/Seraphina-NNUE/master/$nnue_file"

    if [ -f "$nnue_file" ]; then
        echo -e "Seraphina NNUE File already exists -> Skipping download"
    elif command -v wget > /dev/null; then
        echo -e "Downloading ${nnue_file}...${blue}"
        wget -O "$nnue_file" "$url" --no-check-certificate
    elif command -v curl > /dev/null; then
    echo -e "Downloading ${nnue_file}...${blue}"
        curl -o "$nnue_file" "$url"
    else
        echo -e "${red}Error: ${yellow}Neither wget nor curl is installed"
        return 1
    fi

    echo -e "${cyan}---NNUE Download Finished---\n${default}"
    return 0
}

get_nnue