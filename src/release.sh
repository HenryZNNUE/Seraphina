cyan='\e[1;96m'
blue='\e[1;94m'
red='\e[1;91m'
yellow='\e[1;93m'
default='\e[0m'

function release {
    cd scripts

    echo -e "${blue}Building Release Recipe..."
    sh build-recipe.sh
    echo -e "---Release Recipe Build Finished---${default}"
    ./Recipe

    sh recipe.sh
}

release