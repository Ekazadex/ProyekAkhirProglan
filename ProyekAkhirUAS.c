/* 
Proyek Akhir Proglan : 
Sistem Reservasi Tiket Taman Margasatwa Ragunan

Grup 21 :
- Ekananda Zhafif Dean 2306264420
- M Iqbal Al-Fajri 2306250705
22 Mei 2024 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>

#define MAX_TICKETS 100
#define FILENAME "bookings.txt" // Nama file untuk menyimpan data booking

// Struktur data untuk booking
typedef struct Booking {
    char name[50];
    char date[50];
    char attractions[200];
    char purchase_time[50];
    int adult_tickets;
    int child_tickets;
    float total_price;
    struct Booking *next;
} Booking;

// Deklarasi variabel global
Booking *head = NULL;
int tickets_sold = 0;
omp_lock_t lock;

// Deklarasi fungsi-fungsi
void addBooking(char *name, char *date, char *attraction, int adult_tickets, int child_tickets, float total_price);
void printTicket(Booking *booking);
Booking* searchBooking(char *query);
void cancelBooking(char *name);
void displayAllBookings();
void displayTicketPrices();
void displayAttractionPrices();
void displayMenu();
void saveBookingsToFile();
void loadBookingsFromFile();
int compareDates(char *date1, char *date2);
void swapBookings(Booking *a, Booking *b);
void sortBookingsByDate();
void getCurrentTime(char *buffer, int size);
void printTableHeader();
void printTableRow(Booking *booking);

// Fungsi untuk mendapatkan waktu saat ini dalam format yang diinginkan
void getCurrentTime(char *buffer, int size) {
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, size, "%d-%m-%Y %H:%M:%S", timeinfo);
}

// Fungsi untuk menyimpan data booking ke file
void saveBookingsToFile() {
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL) {
        printf("Gagal membuka file untuk penyimpanan.\n");
        return;
    }

    Booking *temp = head;
    while (temp != NULL) {
        fprintf(file, "%s|%s|%s|%s|%d|%d|%.2f\n", temp->name, temp->date, temp->attractions, temp->purchase_time, temp->adult_tickets, temp->child_tickets, temp->total_price);
        temp = temp->next;
    }

    fclose(file);
}

// Fungsi untuk memuat data booking dari file
void loadBookingsFromFile() {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        printf("File tidak ditemukan atau kosong.\n");
        return;
    }

    char name[50], date[50], attractions[200], purchase_time[50];
    int adult_tickets, child_tickets;
    float total_price;
    while (fscanf(file, "%49[^|]|%49[^|]|%199[^|]|%49[^|]|%d|%d|%f\n", name, date, attractions, purchase_time, &adult_tickets, &child_tickets, &total_price) != EOF) {
        Booking *new_booking = (Booking*)malloc(sizeof(Booking));
        strcpy(new_booking->name, name);
        strcpy(new_booking->date, date);
        strcpy(new_booking->attractions, attractions);
        new_booking->adult_tickets = adult_tickets;
        new_booking->child_tickets = child_tickets;
        new_booking->total_price = total_price;
        strcpy(new_booking->purchase_time, purchase_time);
        new_booking->next = NULL;

        omp_set_lock(&lock);
        if (head == NULL) {
            head = new_booking;
        } else {
            Booking *temp = head;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = new_booking;
        }
        tickets_sold++;
        omp_unset_lock(&lock);
    }

    fclose(file);
}

// Fungsi untuk menambah booking
void addBooking(char *name, char *date, char *attractions, int adult_tickets, int child_tickets, float total_price) {
    Booking *new_booking = (Booking*)malloc(sizeof(Booking));
    if (new_booking == NULL) {
        printf("Gagal mengalokasikan memori untuk booking baru.\n");
        return;
    }

    strcpy(new_booking->name, name);
    strcpy(new_booking->date, date);
    strcpy(new_booking->attractions, attractions);
    getCurrentTime(new_booking->purchase_time, sizeof(new_booking->purchase_time));
    new_booking->adult_tickets = adult_tickets;
    new_booking->child_tickets = child_tickets;
    new_booking->total_price = total_price;
    new_booking->next = NULL;

    omp_set_lock(&lock);
    if (head == NULL) {
        head = new_booking;
    } else {
        Booking *temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_booking;
    }
    tickets_sold++;
    omp_unset_lock(&lock);
}

// Fungsi untuk mencetak tiket
void printTicket(Booking *booking) {
    printf("\n--- Tiket ---\n");
    printf("Nama: %s\n", booking->name);
    printf("Tanggal: %s\n", booking->date);

    // Membuat salinan sementara dari string attractions
    char temp_attractions[200];
    strcpy(temp_attractions, booking->attractions);

    // Memisahkan atraksi menjadi token menggunakan strtok
    char *token = strtok(temp_attractions, ",");
    printf("Atraksi: ");
    while (token != NULL) {
        printf("%s, ", token);
        token = strtok(NULL, ",");
    }
    printf("\nJumlah Tiket Dewasa: %d\n", booking->adult_tickets);
    printf("Jumlah Tiket Anak-anak: %d\n", booking->child_tickets);
    printf("Total Harga: %.2f\n", booking->total_price);
    printf("Waktu Pembelian: %s\n", booking->purchase_time);
    printf("--------------\n");
}

// Fungsi untuk mencari booking berdasarkan nama atau tanggal
Booking* searchBooking(char *query) {
    omp_set_lock(&lock);
    Booking *temp = head;
    while (temp != NULL) {
        if (strcmp(temp->name, query) == 0 || strcmp(temp->date, query) == 0) {
            omp_unset_lock(&lock);
            return temp;
        }
        temp = temp->next;
    }
    omp_unset_lock(&lock);
    return NULL;
}

// Fungsi untuk membatalkan booking
void cancelBooking(char *name) {
    omp_set_lock(&lock);
    Booking *temp = head;
    Booking *prev = NULL;
    while (temp != NULL) {
        if (strcmp(temp->name, name) == 0) {
            if (prev == NULL) {
                head = temp->next;
            } else {
                prev->next = temp->next;
            }
            free(temp);
            tickets_sold--;
            omp_unset_lock(&lock);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
    omp_unset_lock(&lock);
    printf("Booking tidak ditemukan.\n");
}

// Fungsi untuk menampilkan semua informasi pembeli dalam bentuk tabel
void displayAllBookings() {
    omp_set_lock(&lock);
    printTableHeader();
    Booking *temp = head;
    while (temp != NULL) {
        printTableRow(temp);
        temp = temp->next;
    }
    omp_unset_lock(&lock);
}

// Fungsi untuk menampilkan header tabel
void printTableHeader() {
    printf("Daftar Pembeli Tiket Kebun Binatang Margasatwa Ragunan:\n");
    printf("---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    printf("| %-20s | %-10s | %-50s | %-20s | %-16s | %-16s | %-10s |\n", "Nama", "Tanggal", "Atraksi", "Waktu Pembelian", "Tiket Dewasa", "Tiket Anak-anak", "Total Harga");
    printf("---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
}

// Fungsi untuk menampilkan baris tabel
void printTableRow(Booking *booking) {
    printf("| %-20s | %-10s | %-50s | %-20s | %-16d | %-16d | %-11.2f |\n", booking->name, booking->date, booking->attractions, booking->purchase_time, booking->adult_tickets, booking->child_tickets, booking->total_price);
    printf("---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
}

// Fungsi untuk menampilkan tabel harga tiket masuk
void displayTicketPrices() {
    printf("\nHarga Tiket Masuk:\n");
    printf("----------------------\n");
    printf("| Kategori  |  Harga  |\n");
    printf("-----------------------\n");
    printf("| Dewasa    | Rp4,000 |\n");
    printf("| Anak-anak | Rp3,000 |\n");
    printf("-----------------------\n");
}

// Fungsi untuk menampilkan tabel harga atraksi
void displayAttractionPrices() {
    printf("\nHarga Atraksi:\n");
    printf("------------------------------------------------\n");
    printf("| Atraksi                  | Harga             |\n");
    printf("------------------------------------------------\n");
    printf("| Gajah Tunggang           | Rp20,000          |\n");
    printf("| Kuda Tunggang            | Rp10,000          |\n");
    printf("| Unta Tunggang            | Rp10,000          |\n");
    printf("| Kereta Keliling          | Rp10,000          |\n");
    printf("| Sewa Sepeda              | Rp10,000/jam      |\n");
    printf("| Perahu Angsa             | Rp15,000          |\n");
    printf("| Pentas Satwa             | Rp5,000           |\n");
    printf("| Pusat Primata Schmutzer  | Rp6,000 (weekdays)|\n");
    printf("|                          | Rp7,500 (weekend) |\n");
    printf("------------------------------------------------\n");
}

// Fungsi untuk menampilkan menu
void displayMenu() {
    printf("-----------------------------------------------------\n");
    printf("|                      Menu:                        |\n");
    printf("-----------------------------------------------------\n");
    printf("| 1. Beli Tiket Masuk                                |\n");
    printf("| 2. Lihat Harga Tiket dan Atraksi                   |\n");
    printf("| 3. Tampilkan Informasi Pembeli                     |\n");
    printf("| 4. Cari Informasi Berdasarkan Nama atau Tanggal    |\n");
    printf("| 5. Urutkan Informasi Pembeli                       |\n");
    printf("| 6. Cancel Booking                                  |\n");
    printf("| 7. Tampilkan Sisa Tiket                            |\n");
    printf("| 8. Keluar                                          |\n");
    printf("-----------------------------------------------------\n");
}

// Fungsi untuk membandingkan dua tanggal dalam format DD-MM-YYYY
int compareDates(char *date1, char *date2) {
    int day1, month1, year1, day2, month2, year2;
    sscanf(date1, "%d-%d-%d", &day1, &month1, &year1);
    sscanf(date2, "%d-%d-%d", &day2, &month2, &year2);
    
    if (year1 != year2) {
        return year1 - year2;
    } else if (month1 != month2) {
        return month1 - month2;
    } else {
        return day1 - day2;
    }
}

// Fungsi untuk menukar dua booking
void swapBookings(Booking *a, Booking *b) {
    char temp_name[50], temp_date[50], temp_attractions[200], temp_purchase_time[50];
    int temp_adult_tickets, temp_child_tickets;
    float temp_total_price;

    strcpy(temp_name, a->name);
    strcpy(temp_date, a->date);
    strcpy(temp_attractions, a->attractions);
    strcpy(temp_purchase_time, a->purchase_time);
    temp_adult_tickets = a->adult_tickets;
    temp_child_tickets = a->child_tickets;
    temp_total_price = a->total_price;

    strcpy(a->name, b->name);
    strcpy(a->date, b->date);
    strcpy(a->attractions, b->attractions);
    strcpy(a->purchase_time, b->purchase_time);
    a->adult_tickets = b->adult_tickets;
    a->child_tickets = b->child_tickets;
    a->total_price = b->total_price;

    strcpy(b->name, temp_name);
    strcpy(b->date, temp_date);
    strcpy(b->attractions, temp_attractions);
    strcpy(b->purchase_time, temp_purchase_time);
    b->adult_tickets = temp_adult_tickets;
    b->child_tickets = temp_child_tickets;
    b->total_price = temp_total_price;
}

// Fungsi untuk sorting informasi pembeli berdasarkan tanggal
void sortBookingsByDate() {
    if (head == NULL) return;

    int swapped;
    Booking *current;
    Booking *prev = NULL;
    Booking *tempHead = head;

    do {
        swapped = 0;
        current = tempHead;

        while (current->next != prev) {
            if (compareDates(current->date, current->next->date) > 0) {
                // Swap the bookings
                swapBookings(current, current->next);
                swapped = 1;
            }
            current = current->next;
        }
        prev = current;
    } while (swapped);
}


int main() {
    omp_init_lock(&lock);

    loadBookingsFromFile(); // Memuat data booking dari file sebelumnya

    int choice;
    char name[50], date[50], attractions[200], add_attraction;
    float total_price;
    int adult_tickets, child_tickets;

    do {
        system("cls");
        displayMenu();
        printf("Pilih menu: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                // Kasus 1: Beli Tiket Masuk
                system("cls");
                if (tickets_sold >= MAX_TICKETS) {
                    printf("Tiket sudah habis.\n");
                    break;
                }
                printf("Masukkan nama: ");
                scanf(" %[^\n]%*c", name); // Menggunakan format specifier %[^\n]%*c untuk membaca string hingga akhir baris
                printf("Masukkan tanggal (DD-MM-YYYY): ");
                scanf("%s", date);
                
                printf("Tambah atraksi? (y/n): ");
                scanf(" %c", &add_attraction);
                
                if (add_attraction == 'y' || add_attraction == 'Y') {
                    // Menampilkan tabel atraksi
                    displayAttractionPrices();
                    printf("Pilih atraksi (pisahkan dengan koma):\n");
                    scanf(" %[^\n]%*c", attractions);
                } else {
                    strcpy(attractions, "Tidak ada atraksi tambahan");
                }

                printf("Jumlah tiket dewasa: ");
                scanf("%d", &adult_tickets);
                printf("Jumlah tiket anak-anak: ");
                scanf("%d", &child_tickets);

                // Kalkulasi harga total berdasarkan jumlah tiket dan atraksi
                total_price = (adult_tickets * 4000) + (child_tickets * 3000); // Harga tiket masuk
                if (strstr(attractions, "Gajah Tunggang")) total_price += (adult_tickets + child_tickets) * 20000;
                if (strstr(attractions, "Kuda Tunggang")) total_price += (adult_tickets + child_tickets) * 10000;
                if (strstr(attractions, "Unta Tunggang")) total_price += (adult_tickets + child_tickets) * 10000;
                if (strstr(attractions, "Kereta Keliling")) total_price += (adult_tickets + child_tickets) * 10000;
                if (strstr(attractions, "Sewa Sepeda")) total_price += (adult_tickets + child_tickets) * 10000;
                if (strstr(attractions, "Perahu Angsa")) total_price += (adult_tickets + child_tickets) * 15000;
                if (strstr(attractions, "Pentas Satwa")) total_price += (adult_tickets + child_tickets) * 5000;
                if (strstr(attractions, "Pusat Primata Schmutzer")) {
                    if (date[0] == '0' || date[0] == '6') // Check if it's a weekend (Saturday or Sunday)
                        total_price += (adult_tickets + child_tickets) * 7500; // Weekend price
                    else
                        total_price += (adult_tickets + child_tickets) * 6000; // Weekday price
                }
                
                addBooking(name, date, attractions, adult_tickets, child_tickets, total_price);
                Booking *new_booking = searchBooking(name);
                printTicket(new_booking);
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
            case 2:
                // Kasus 2: Lihat Harga Tiket dan Atraksi
                system("cls");
                displayTicketPrices();
                displayAttractionPrices();
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
            case 3:
                // Kasus 3: Tampilkan Informasi Pembeli
                system("cls");
                displayAllBookings();
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
            case 4:
                // Kasus 4: Cari Informasi Berdasarkan Nama atau Tanggal
                system("cls");
                printf("Masukkan nama atau tanggal: ");
                scanf(" %[^\n]%*c", name);
                Booking *found_booking = searchBooking(name);
                if (found_booking != NULL) {
                    // Display the header and row for the found booking in table format
                    printf("\n");
                    printTableHeader();
                    printTableRow(found_booking);
                } else {
                    printf("Booking tidak ditemukan.\n");
                }
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
            case 5:
                // Kasus 5: Urutkan Informasi Pembeli
                system("cls");
                sortBookingsByDate();
                printf("Informasi pembeli telah diurutkan berdasarkan tanggal.\n");
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
            case 6:
                // Kasus 6: Cancel Booking
                system("cls");
                printf("Masukkan nama untuk cancel booking: ");
                scanf(" %[^\n]%*c", name);
                cancelBooking(name);
                printf("Informasi Pembeli Atas Nama %s Telah Dihapus.\n", name);
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break; 
            case 7:
                // Kasus 7: Tampilkan Sisa Tiket
                system("cls");
                printf("Sisa tiket: %d\n", MAX_TICKETS - tickets_sold);
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
            case 8:
                // Kasus 8: Keluar
                system("cls");
                saveBookingsToFile(); // Menyimpan data booking ke file sebelum keluar
                omp_destroy_lock(&lock);
                exit(0);
            default:
                system("cls");
                printf("Pilihan tidak valid.\n");
                printf("Tekan enter untuk kembali ke menu utama...");
                fflush(stdin);
                getchar();
                break;
        }
    } while (choice != 8);
    
    return 0;
}