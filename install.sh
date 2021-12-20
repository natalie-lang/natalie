echo "\e[0;34m [Installing Natalie, Please wait...]"

git clone https://github.com/seven1m/natalie
cd natalie
rake
cd bin
chmod +x natalie
sudo mv natalie /usr/bin/
