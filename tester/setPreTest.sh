# Step 1
# Before starting please follow the next few steps (files content can be anything and will be shown to you by the test):
# - Download the cgi_test executable on the host
# - Create a directory YoupiBanane with:
#         -a file name youpi.bad_extension
#         -a file name youpi.bla
#         -a sub directory called nop
#                 -a file name youpi.bad_extension in nop
#                 -a file name other.pouic in nop
#         -a sub directory called Yeah
#                 -a file name not_happy.bad_extension in Yeah
# press enter to continue



if [ ! -f "tester" ] && [ ! -f "cgi_tester" ]; then

    echo "One or both of the files('tester' and 'cgi_tester') do not exist."
    exit 1
fi

echo "Both files 'tester' and 'cgi_tester' exist."

# Function to generate the next backup name
get_next_backup_name() {
    local dir_name=$1
    local counter=1
    while [[ -e "${dir_name}_backup${counter}" ]]; do
        let counter++
    done
    echo "${dir_name}_backup${counter}"
}

dir_name="YoupiBanane"

if [[ -d "${dir_name}" ]]; then
    # Directory exists, rename it with a suffix
    backup_name=$(get_next_backup_name "${dir_name}")
    mv "${dir_name}" "${backup_name}"
    echo "Directory '${dir_name}' already exists. Renamed to '${backup_name}'."
fi

mkdir "${dir_name}" && cd "${dir_name}" &&
    touch youpi.bad_extension &&
    touch youpi.bla &&
    mkdir nop &&  cd nop &&
        touch youpi.bad_extension &&
        touch other.pouic && cd .. &&
    mkdir Yeah && cd Yeah
        touch not_happy.bad_extension
echo "Setting O.K"

# cd ../..


# Setup the configuration file as follow:
# - / must answer to GET request ONLY
# - /put_test/* must answer to PUT request and save files to a directory of your choice
# - any file with .bla as extension must answer to POST request by calling the cgi_test executable
# - /post_body must answer anything to POST request with a maxBody of 100
# - /directory/ must answer to GET request and the root of it would be the repository YoupiBanane and if no file are requested, it should search for youpi.bad_extension files

# FOR tester.conf
mkdir YoupiBanane/put_test

cat > tester.conf << EOF
# Step 1
# Before starting please follow the next few steps (files content can be anything and will be shown to you by the test):
# - Download the cgi_test executable on the host
# - Create a directory YoupiBanane with:
#         -a file name youpi.bad_extension
#         -a file name youpi.bla
#         -a sub directory called nop
#                 -a file name youpi.bad_extension in nop
#                 -a file name other.pouic in nop
#         -a sub directory called Yeah
#                 -a file name not_happy.bad_extension in Yeah
# press enter to continue

# Step2
# Setup the configuration file as follow:
# - / must answer to GET request ONLY
# - /put_test/* must answer to PUT request and save files to a directory of your choice
# - any file with .bla as extension must answer to POST request by calling the cgi_test executable
# - /post_body must answer anything to POST request with a maxBody of 100
# - /directory/ must answer to GET request and the root of it would be the repository YoupiBanane and if no file are requested, it should search for youpi.bad_extension files
server {
    listen 8080; // 나 편한대로

    root YoupiBanane;

    location / {
        limit_except GET;
    }

    location /put_test/ {
        root YoupiBanane/put_test;
        limit_except PUT;
    }

    location /post_body {
        limit_except PUT;
        limit_client_body_size 100;
    }

    location /directory/ {
        root /YoupiBanane/;
		allow_method GET,POST;
        index youpi.bad_extension;
    }
}
