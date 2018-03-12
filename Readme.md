## IPK - Klient-server pro získání informace o uživatelích
# Informace
Jedná se o aplikaci typu klient-server pro získání informací o uživatelích ze souboru /etc/passwd.
# Překlad
 - Projekt lze přeložit příkazem **make**, který vytvoří dva spustitelné
   soubory (ipk-client pro klienta a ipk-server pro server).
 - Pomocí příkazu **make clean** lze odstranit objektové soubory
   a příkazem **make remove** navíc i oba spustitelné soubory.
# Server
    ./ipk-server -p port
 - _port_ (číslo) číslo portu, na kterém server naslouchá na připojení od klientů.
# Klient
     ./ipk-client -h host -p port [-n|-f|-l] login
 - _host_ (IP adresa nebo fully-qualified DNS name) identifikace serveru jakožto koncového bodu komunikace klienta;
 - _port_ (číslo) cílové číslo portu;
 - _-n_ značí, že bude vráceno plné jméno uživatele včetně případných dalších informací pro uvedený login (User ID Info);
 - _-f_ značí, že bude vrácena informace o domácím adresáři uživatele pro uvedený login (Home directory);
 - _-l_ značí, že bude vrácen seznam všech uživatelů, tento bude vypsán tak, že každé uživatelské jméno
 bude na zvláštním řádku; v tomto případě je login nepovinný. Je-li však uveden bude použit jako prefix pro výběr uživatelů.
 - _login_ určuje přihlašovací jméno uživatele pro výše uvedené operace.
