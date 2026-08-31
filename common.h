// Header guard
#ifndef COMMON_H
#define COMMON_H


// Check if using Windows
#ifdef _WIN32

	// System version
	#define _WIN32_WINNT _WIN32_WINNT_WIN10
	
	// Use Unicode
	#define UNICODE
	#define _UNICODE
	
	// Header files
	#include <ws2tcpip.h>
	#include <setupapi.h>
	#include <initguid.h>
	#include <emi.h>
	#include <shlwapi.h>
	
	// Check if displaying power usage
	#if DISPLAY_POWER_USAGE
	
		// Header files
		#include <nvml.h>
	#endif
	
// Otherwise check if using an Apple device
#elif defined __APPLE__

	// Use bounds-checking interfaces
	#define __STDC_WANT_LIB_EXT1__ 1
	
	// Use Metal
	#define MTL_PRIVATE_IMPLEMENTATION
	
	// Use NS
	#define NS_PRIVATE_IMPLEMENTATION
	
	// Header files
	#include <IOKit/IOKitLib.h>
	#include <IOKit/pwr_mgt/IOPMLib.h>
	#include <mach/thread_act.h>
	#include <netdb.h>
	#include <netinet/tcp.h>
	#include <sys/sysctl.h>
	
// Otherwise
#else

	// Header files
	#include <netdb.h>
	#include <netinet/tcp.h>
	
	// Check if displaying power usage
	#if DISPLAY_POWER_USAGE
	
		// Header files
		#include <nvml.h>
	#endif
	
	// Check if preventing sleep
	#if PREVENT_SLEEP
	
		// Header files
		#include <dbus/dbus.h>
	#endif
#endif

// Header files
#include <cstring>
#include <bit>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <dirent.h>
#include <thread>

using namespace std;


// Constants

// Check if nonce is a uint8
#if NONCE_SIZE == 1

	// Nonce type
	typedef uint8_t NonceType;
	
// Otherwise check if nonce is a uint16
#elif NONCE_SIZE == 2

	// Nonce type
	typedef uint16_t NonceType;
	
// Otherwise check if nonce is a uint32
#elif NONCE_SIZE == 4

	// Nonce type
	typedef uint32_t NonceType;
	
// Otherwise nonce is a uint64
#else

	// Nonce type
	typedef uint64_t NonceType;
#endif

// Min edge bits
#define MIN_EDGE_BITS 10

// Max edge bits
#define MAX_EDGE_BITS 32

// Minimum additional space tolerance percent
#define MINIMUM_ADDITIONAL_SPACE_TOLERANCE_PERCENT 0.02275

// Check if CPU number of most significant bits used for fine bucket sorting is at most six
#if CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING <= 6

	// Additional space tolerance scale factor
	#define ADDITIONAL_SPACE_TOLERANCE_SCALE_FACTOR 0.008
	
// Otherwise
#else

	// Additional space tolerance scale factor
	#define ADDITIONAL_SPACE_TOLERANCE_SCALE_FACTOR 0.0145
#endif

// Percent of edges remaining after one trimming round
#define PERCENT_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND 0.632151

// Number of edges remaining after one trimming round additional tolerance percent
#define NUMBER_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND_ADDITIONAL_TOLERANCE_PERCENT 0.1

// Number of edges
#define NUMBER_OF_EDGES (static_cast<uint64_t>(1) << EDGE_BITS)

// Trimming rounds
#define TRIMMING_ROUNDS (GPU_TRIMMING_ROUNDS + CPU_TRIMMING_ROUNDS)

// Max number of edges after trimming
#define MAX_NUMBER_OF_EDGES_AFTER_TRIMMING maxNumberOfEdgesRemainingAfterTimmingRounds(TRIMMING_ROUNDS)

// Number of edge components
#define NUMBER_OF_EDGE_COMPONENTS 2

// Bits in a byte
#define BITS_IN_A_BYTE 8

// Bytes in a kilobyte
#define BYTES_IN_A_KILOBYTE 1024

// Kilobytes in a megabyte
#define KILOBYTES_IN_A_MEGABYTE BYTES_IN_A_KILOBYTE

// Megabytes in a gigabyte
#define MEGABYTES_IN_A_GIGABYTE BYTES_IN_A_KILOBYTE

// Bytes in a gigabyte
#define BYTES_IN_A_GIGABYTE (BYTES_IN_A_KILOBYTE * KILOBYTES_IN_A_MEGABYTE * MEGABYTES_IN_A_GIGABYTE)

// Seconds in a minute
#define SECONDS_IN_A_MINUTE 60

// Minutes in an hour
#define MINUTES_IN_AN_HOUR 60

// Milliseconds in a second
#define MILLISECONDS_IN_A_SECOND 1000

// Milliseconds in a second
#define MICROSECONDS_IN_A_MILLISECOND 1000

// Nanoseconds in a second
#define NANOSECONDS_IN_A_SECOND 1000000000

// Nanojoules in a joule
#define NANOJOULES_IN_A_JOULE 1000000000

// Nanojoules in a millijoule
#define NANOJOULES_IN_A_MILLIJOULE 1000000

// Nanojoules in a microjoule
#define NANOJOULES_IN_A_MICROJOULE 1000

// Picojoules in a nanojoule
#define PICOJOULES_IN_A_NANOJOULE 1000

// Nanowatts in a watt
#define NANOWATTS_IN_A_WATT 1000000000

// picojoule in a picowatt-hours
#define PICOJOULES_IN_A_PICOWATT_HOUR (SECONDS_IN_A_MINUTE * MINUTES_IN_AN_HOUR)

// Decimal number base
#define DECIMAL_NUMBER_BASE 10

// Hexadecimal character size
#define HEXADECIMAL_CHARACTER_SIZE (sizeof("FF") - sizeof('\0'))

// Max uint32 string size
#define MAX_UINT32_STRING_SIZE (sizeof("4294967295") - sizeof('\0'))

// Max uint64 string size
#define MAX_UINT64_STRING_SIZE (sizeof("18446744073709551615") - sizeof('\0'))

// UUID size
#define UUID_SIZE 16

// To string
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

// Check if using Windows
#ifdef _WIN32

	// Set buffer guaranteed
	#define setBufferGuaranteed(buffer, value, size) (__builtin_expect(value, false) ? __builtin_memset(buffer, value, size) : SecureZeroMemory(buffer, size))
	
// Otherwise check if using an Apple device
#elif defined __APPLE__

	// Set buffer guaranteed
	#define setBufferGuaranteed(buffer, value, size) memset_s(buffer, size, value, size)
	
// Otherwise
#else

	// Set buffer guaranteed
	#define setBufferGuaranteed(buffer, value, size) memset_explicit(buffer, value, size)
#endif

// SMC poll rate microseconds (SMC values are updated every 500ms, so poll at twice that rate)
#define SMC_POLL_RATE_MICROSECONDS (250 * MICROSECONDS_IN_A_MILLISECOND)

// SMC selectors
enum SmcSelectors {

	// Client open
	kSMCUserClientOpen = 0,
	
	// Client close
	kSMCUserClientClose = 1,
	
	// Handle event
	kSMCHandleYPCEvent = 2,
	
	// Read key
	kSMCReadKey = 5,
	
	// Write key
	kSMCWriteKey = 6,
	
	// Key key count
	kSMCGetKeyCount = 7,
	
	// Get key from index
	kSMCGetKeyFromIndex = 8,
	
	// Get key info
	kSMCGetKeyInfo = 9
};

// GNOME inhibit suspending session
#define GNOME_INHIBIT_SUSPENDING_SESSION (1 << 2)


// Structures

// SMC parameters structure
struct SmcParameters {

	// Key
	uint32_t key;
	
	// Version
	struct {
	
		// Major
		uint8_t major;
		
		// Minor
		uint8_t minor;
		
		// Build
		uint8_t build;
		
		// Reserved
		uint8_t reserved;
		
		// Release
		uint16_t release;
		
	} version;
	
	// Limit data
	struct {
	
		// Version
		uint16_t version;
		
		// Length
		uint16_t length;
		
		// CPU limit
		uint32_t cpuLimit;
		
		// GPU limit
		uint32_t gpuLimit;
		
		// Mem limit
		uint32_t memLimit;
		
	} limitData;
	
	// Key info
	struct {
	
		// Data size
		uint32_t dataSize;
		
		// Data type
		uint32_t dataType;
		
		// Data attributes
		uint8_t dataAttributes;
		
	} keyInfo;
	
	// Result
	uint8_t result;
	
	// Status
	uint8_t status;
	
	// Data 8
	uint8_t data8;
	
	// Data 32
	uint32_t data32;
	
	// Bytes
	uint8_t bytes[32];
};


// Classes

// To string class
template<const uint64_t number> class toString final {

	// Private
	private:
	
		// Set buffer
		static constexpr const array buffer = []() __attribute__((always_inline)) constexpr noexcept {
		
			// Get number of digits
			constexpr const size_t numberOfDigits = []() __attribute__((always_inline)) constexpr noexcept {
			
				// Set number of digits to zero
				size_t numberOfDigits = 0;
				
				// Go through all digits in the number
				uint64_t remainingDigits = number;
				do [[likely]] {
				
					// Increment number of digits
					++numberOfDigits;
					
					// Go to the next digit
					remainingDigits /= DECIMAL_NUMBER_BASE;
					
				} while(remainingDigits);
				
				// Return number of digits
				return numberOfDigits;
			}();
			
			// Create buffer
			array<char, numberOfDigits + sizeof('\0')> buffer;
			
			// Go through all digits in the number
			uint64_t remainingDigits = number;
			for(size_t i = 0; i < numberOfDigits; ++i) [[likely]] {
			
				// Set character in the buffer to the digit's character
				buffer[numberOfDigits - 1 - i] = remainingDigits % DECIMAL_NUMBER_BASE + '0';
				
				// Go to the next digit
				remainingDigits /= DECIMAL_NUMBER_BASE;
			}
			
			// Make buffer a string
			buffer[numberOfDigits] = '\0';
			
			// Return buffer
			return buffer;
		}();
		
	// Public
	public:
	
		// Set value to the buffer's data
		static constexpr const string_view value{buffer.data(), buffer.size() - sizeof('\0')};
		
		// Set C string to the buffer's data
		static constexpr const char *cString = buffer.data();
};

// Concatenate strings class
template<const string_view *strings, const size_t numberOfStrings> class concatenateStrings final {

	// Private
	private:
	
		// Set buffer
		static constexpr const array buffer = []() __attribute__((always_inline)) constexpr noexcept {
		
			// Get number of characters
			constexpr const size_t numberOfCharacters = []() __attribute__((always_inline)) constexpr noexcept {
			
				// Set number of characters to zero
				size_t numberOfCharacters = 0;
				
				// Go through all strings
				for(size_t i = 0; i < numberOfStrings; ++i) [[likely]] {
				
					// Add strings number of characters to the number of characters
					numberOfCharacters += strings[i].size();
				}
				
				// Return number of characters
				return numberOfCharacters;
			}();
			
			// Create buffer
			array<char, numberOfCharacters + sizeof('\0')> buffer;
			
			// Go through all strings
			size_t index = 0;
			for(size_t i = 0; i < numberOfStrings; ++i) [[likely]] {
			
				// Go through all characters in the string
				for(const char character : strings[i]) [[likely]] {
				
					// Set character in the buffer
					buffer[index++] = character;
				}
				
			}
			
			// Make buffer a string
			buffer[index] = '\0';
			
			// Return buffer
			return buffer;
		}();
		
	// Public
	public:
	
		// Set value to the buffer's data
		static constexpr const char *value = buffer.data();
};

// Check if preventing sleep
#if PREVENT_SLEEP

	// Prevent sleep
	class PreventSleep final {
	
		// Public
		public:
		
			// Constructor
			__attribute__((always_inline)) inline explicit PreventSleep() noexcept;
			
			// Destructor
			__attribute__((always_inline)) inline ~PreventSleep() noexcept;
			
			// Bool operator
			__attribute__((always_inline)) inline explicit operator bool() const noexcept;
			
		// Private
		private:
		
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Assertion ID
				IOPMAssertionID assertionID;
				
			// Otherwise check if not using Windows
			#elif !defined _WIN32
			
				// Allow sleep message
				unique_ptr<DBusMessage, decltype(&dbus_message_unref)> allowSleepMessage;
			#endif
			
			// Error occurred
			const bool errorOccurred;
	};
#endif

// Check if Windows
#ifdef _WIN32

	// Windows socket class
	class WindowsSocket final {
	
		// Public
		public:
		
			// Constructor
			__attribute__((always_inline)) inline explicit WindowsSocket() noexcept;
			
			// Destructor
			__attribute__((always_inline)) inline ~WindowsSocket() noexcept;
			
			// Bool operator
			__attribute__((always_inline)) inline explicit operator bool() const noexcept;
			
		// Private
		private:
		
			// Major version
			static constexpr const BYTE MAJOR_VERSION = 2;
			
			// Minor version
			static constexpr const BYTE MINOR_VERSION = 2;
			
			// Error occurred
			const bool errorOccurred;
	};
#endif

// Check if displaying power usage
#if DISPLAY_POWER_USAGE

	// Energy consumption class
	class EnergyConsumption final {
	
		// Public
		public:
		
			// Constructor
			__attribute__((always_inline)) inline explicit EnergyConsumption() noexcept;
			
			// Destructor
			__attribute__((always_inline)) inline ~EnergyConsumption() noexcept;
			
			// Bool operator
			__attribute__((always_inline)) inline explicit operator bool() const noexcept;
			
			// Set GPU
			__attribute__((always_inline)) inline void setGpu(const char *vendor [[maybe_unused]], const uint8_t gpuUuid [[maybe_unused]] [UUID_SIZE]) noexcept;
			
			// Get total energy consumption
			__attribute__((always_inline)) inline pair<unsigned long long, unsigned long long> getTotalEnergyConsumption() const noexcept;
			
		// Private
		private:
		
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Channels
				const unique_ptr<remove_pointer_t<CFTypeRef>, decltype(&CFRelease)> channels;
				
				// Subscription
				const unique_ptr<remove_pointer_t<CFTypeRef>, decltype(&CFRelease)> subscription;
				
			// Otherwise
			#else
			
				// NVIDIA initialized
				bool nvidiaInitialized;
				
				// NVIDIA device
				nvmlDevice_t nvidiaDevice;
			#endif
			
			// Error occurred
			const bool errorOccurred;
	};
#endif


// Function prototypes

// Ceil as uint32
__attribute__((always_inline)) static inline constexpr uint32_t ceilAsUint32(const double value) noexcept;

// Ceil as uint64
__attribute__((always_inline)) static inline constexpr uint64_t ceilAsUint64(const double value) noexcept;

// Additional space tolerance percent
__attribute__((always_inline)) static inline constexpr double additionalSpaceTolerancePercent(const unsigned int edgeBits) noexcept;

// Max number of edges remaining after trimming rounds
__attribute__((always_inline)) static inline constexpr uint64_t maxNumberOfEdgesRemainingAfterTimmingRounds(const unsigned int numberOfTrimmingRounds, const bool additionalSpaceToleranceBasedOnInitialNumberOfEdges = true) noexcept;

// Set thread priority and affinity
__attribute__((always_inline)) static inline bool setThreadPriorityAndAffinity(unsigned int cpuCoreIndex) noexcept;

// Get number of high performance CPU cores
__attribute__((always_inline)) static inline unsigned int getNumberOfHighPerformanceCpuCores() noexcept;

// Check if displaying power usage and using an Apple device
#if DISPLAY_POWER_USAGE && defined __APPLE__

	// Extern C
	extern "C" {
	
		// IOReport copy channels in groups
		CFTypeRef IOReportCopyChannelsInGroup(CFStringRef group, CFTypeRef a, uint64_t b, uint64_t c, uint64_t d);
		
		// IOReport create subscription
		CFTypeRef IOReportCreateSubscription(void *a, CFTypeRef channels, CFTypeRef *b, uint64_t c, CFTypeRef d);
		
		// IOReport create samples
		CFDictionaryRef IOReportCreateSamples(CFTypeRef subscription, CFTypeRef channels, CFTypeRef a);
		
		// IOReport channel get channel name
		CFStringRef IOReportChannelGetChannelName(CFTypeRef item);
		
		// IOReport channel get unit label
		CFStringRef IOReportChannelGetUnitLabel(CFTypeRef item);
		
		// IOReport simple get integer value
		int64_t IOReportSimpleGetIntegerValue(CFTypeRef item, int32_t a);
	}
#endif


// Supporting function implementation

// Check if preventing sleep
#if PREVENT_SLEEP

	// Prevent sleep constructor
	__attribute__((always_inline)) inline PreventSleep::PreventSleep() noexcept :
	
		// Check if Windows
		#ifdef _WIN32
		
			// Set error occurred to if preventing sleep failed
			errorOccurred(!SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED))
			
		// Otherwise check if using an Apple device
		#elif defined __APPLE__
		
			// Set error occurred to if preventing sleep failed
			errorOccurred(IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep, kIOPMAssertionLevelOn, CFSTR(TO_STRING(NAME) " is running"), &assertionID) != kIOReturnSuccess)
			
		// Otherwise
		#else
		
			// Set allow sleep message to nothing
			allowSleepMessage(nullptr, dbus_message_unref),
			
			// Set error occurred to false
			errorOccurred(false)
		#endif
	{
	
		// Check if not using Windows or an Apple device
		#if !defined _WIN32 && !defined __APPLE__
		
			// Check if connecting to the session bus was successful
			DBusConnection *connection = dbus_bus_get(DBUS_BUS_SESSION, nullptr);
			if(connection) [[likely]] {
			
				// Don't close program if the connection closes
				dbus_connection_set_exit_on_disconnect(connection, false);
				
				// Set application identifier and reason
				const char *applicationIdentifier = TO_STRING(NAME);
				const char *reason = TO_STRING(NAME) " is running";
				
				// Check if creating message to prevent sleep for GNOME-compliant environments was successful
				unique_ptr<DBusMessage, decltype(&dbus_message_unref)> message(dbus_message_new_method_call("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", "Inhibit"), dbus_message_unref);
				if(message) [[likely]] {
				
					// Check if setting message's application identifier, toplevel window identifier, reason, and flags arguments was successful
					if(dbus_message_append_args(message.get(), DBUS_TYPE_STRING, &applicationIdentifier, DBUS_TYPE_UINT32, &static_cast<const dbus_uint32_t &>(dbus_uint32_t(0)), DBUS_TYPE_STRING, &reason, DBUS_TYPE_UINT32, &static_cast<const dbus_uint32_t &>(dbus_uint32_t(GNOME_INHIBIT_SUSPENDING_SESSION)), DBUS_TYPE_INVALID)) [[likely]] {
					
						// Check if getting message's reply was successful
						const unique_ptr<DBusMessage, decltype(&dbus_message_unref)> reply(dbus_connection_send_with_reply_and_block(connection, message.get(), DBUS_TIMEOUT_USE_DEFAULT, nullptr), dbus_message_unref);
						if(reply) [[likely]] {
						
							// Check if getting reply's inhibit cookie was successful
							dbus_uint32_t inhibitCookie;
							if(dbus_message_get_args(reply.get(), nullptr, DBUS_TYPE_UINT32, &inhibitCookie, DBUS_TYPE_INVALID)) [[likely]] {
							
								// Check if creating message to allow sleep was successful
								allowSleepMessage = unique_ptr<DBusMessage, decltype(&dbus_message_unref)>(dbus_message_new_method_call("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", "Uninhibit"), dbus_message_unref);
								if(allowSleepMessage) [[likely]] {
								
									// Check if setting allow sleep message's inhibit cookit argument failed
									if(!dbus_message_append_args(allowSleepMessage.get(), DBUS_TYPE_UINT32, &inhibitCookie, DBUS_TYPE_INVALID)) [[unlikely]] {
									
										// Free allow sleep message
										allowSleepMessage.reset();
									}
									
									// Otherwise
									else [[likely]] {
									
										// Return
										return;
									}
								}
							}
						}
					}
				}
				
				// Check if creating message to prevent sleep for Freedesktop-compliant environments was successful
				message = unique_ptr<DBusMessage, decltype(&dbus_message_unref)>(dbus_message_new_method_call("org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement/Inhibit", "org.freedesktop.PowerManagement.Inhibit", "Inhibit"), dbus_message_unref);
				if(message) [[likely]] {
				
					// Check if setting message's application identifier and reason arguments was successful
					if(dbus_message_append_args(message.get(), DBUS_TYPE_STRING, &applicationIdentifier, DBUS_TYPE_STRING, &reason, DBUS_TYPE_INVALID)) [[likely]] {
					
						// Check if getting message's reply was successful
						const unique_ptr<DBusMessage, decltype(&dbus_message_unref)> reply(dbus_connection_send_with_reply_and_block(connection, message.get(), DBUS_TIMEOUT_USE_DEFAULT, nullptr), dbus_message_unref);
						if(reply) [[likely]] {
						
							// Check if getting reply's inhibit cookie was successful
							dbus_uint32_t inhibitCookie;
							if(dbus_message_get_args(reply.get(), nullptr, DBUS_TYPE_UINT32, &inhibitCookie, DBUS_TYPE_INVALID)) [[likely]] {
							
								// Check if creating message to allow sleep was successful
								allowSleepMessage = unique_ptr<DBusMessage, decltype(&dbus_message_unref)>(dbus_message_new_method_call("org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement/Inhibit", "org.freedesktop.PowerManagement.Inhibit", "UnInhibit"), dbus_message_unref);
								if(allowSleepMessage) [[likely]] {
								
									// Check if setting allow sleep message's inhibit cookit argument failed
									if(!dbus_message_append_args(allowSleepMessage.get(), DBUS_TYPE_UINT32, &inhibitCookie, DBUS_TYPE_INVALID)) [[unlikely]] {
									
										// Free allow sleep message
										allowSleepMessage.reset();
									}
									
									// Otherwise
									else [[likely]] {
									
										// Return
										return;
									}
								}
							}
						}
					}
				}
			}
		#endif
	}
	
	// Prevent sleep destructor
	__attribute__((always_inline)) inline PreventSleep::~PreventSleep() noexcept {
	
		// Check if an error didn't occur
		if(!errorOccurred) [[likely]] {
		
			// Check if using Windows
			#ifdef _WIN32
			
				// Allow sleep
				SetThreadExecutionState(ES_CONTINUOUS);
				
			// Otherwise check if using an Apple device
			#elif defined __APPLE__
			
				// Allow sleep
				IOPMAssertionRelease(assertionID);
				
			// Otherwise
			#else
			
				// Check if allow sleep message exists
				if(allowSleepMessage) [[likely]] {
				
					// Check if connecting to the session bus was successful
					DBusConnection *connection = dbus_bus_get(DBUS_BUS_SESSION, nullptr);
					if(connection) [[likely]] {
					
						// Don't close program if the connection closes
						dbus_connection_set_exit_on_disconnect(connection, false);
						
						// Get allow sleep message's reply
						unique_ptr<DBusMessage, decltype(&dbus_message_unref)>(dbus_connection_send_with_reply_and_block(connection, allowSleepMessage.get(), DBUS_TIMEOUT_USE_DEFAULT, nullptr), dbus_message_unref);
					}
				}
			#endif
		}
	}
	
	// Prevent sleep bool operator
	__attribute__((always_inline)) inline PreventSleep::operator bool() const noexcept {
	
		// Return if an error didn't occurred
		return !errorOccurred;
	}
#endif

// Check if Windows
#ifdef _WIN32

	// Windows socket constructor
	__attribute__((always_inline)) inline WindowsSocket::WindowsSocket() noexcept :
	
		// Set error occurred to if initializing Windows socket failed
		errorOccurred(WSAStartup(MAKEWORD(WindowsSocket::MAJOR_VERSION, WindowsSocket::MINOR_VERSION), const_cast<WSADATA *>(&static_cast<const WSAData &>(WSAData()))))
	{
	}
	
	// Windows socket destructor
	__attribute__((always_inline)) inline WindowsSocket::~WindowsSocket() noexcept {
	
		// Check if an error didn't occur
		if(!errorOccurred) [[likely]] {
		
			// Clean up Windows socket
			WSACleanup();
		}
	}
	
	// Windows socket bool operator
	__attribute__((always_inline)) inline WindowsSocket::operator bool() const noexcept {
	
		// Return if an error didn't occurred
		return !errorOccurred;
	}
#endif

// Check if displaying power usage
#if DISPLAY_POWER_USAGE

	// Energy consumption constructor
	__attribute__((always_inline)) inline EnergyConsumption::EnergyConsumption() noexcept :
	
		// Check if using an Apple device
		#ifdef __APPLE__
		
			// Get energy model channels
			channels(IOReportCopyChannelsInGroup(CFSTR("Energy Model"), nullptr, 0, 0, 0), CFRelease),
			
			// Get subscription to channels
			subscription(IOReportCreateSubscription(nullptr, channels.get(), const_cast<CFTypeRef *>(&static_cast<const CFTypeRef &>(CFTypeRef())), 0, nullptr), CFRelease),
			
			// Set error occurred to if getting channels or subscription failed
			errorOccurred(!channels || !subscription)
			
		// Otherwise
		#else
		
			// Set NVIDIA initialize to false
			nvidiaInitialized(false),
			
			// Set error occurred to false
			errorOccurred(false)
		#endif
	{
	}
	
	// Destructor
	__attribute__((always_inline)) inline EnergyConsumption::~EnergyConsumption() noexcept {
	
		// Check if not using an Apple device
		#ifndef __APPLE__
		
			// Check if NVIDIA is initialized
			if(nvidiaInitialized) [[likely]] {
			
				// Shutdown NVIDIA
				nvmlShutdown();
			}
		#endif
	}
	
	// Energy consumption bool operator
	__attribute__((always_inline)) inline EnergyConsumption::operator bool() const noexcept {
	
		// Return if an error didn't occurred
		return !errorOccurred;
	}
	
	// Energy consumption set GPU
	__attribute__((always_inline)) inline void EnergyConsumption::setGpu(const char *vendor [[maybe_unused]], const uint8_t gpuUuid [[maybe_unused]] [UUID_SIZE]) noexcept {
	
		// Check if not using an Apple device
		#ifndef __APPLE__
		
			// Check if Windows
			#ifdef _WIN32
			
				// Check if GPU is a NVIDIA GPU
				if(StrStrIA(vendor, "NVIDIA")) [[likely]] {
				
			// Otherwise
			#else
			
				// Check if GPU is a NVIDIA GPU
				if(strcasestr(vendor, "NVIDIA")) [[likely]] {
			#endif
			
				// Check if initializing NVIDIA was successful
				nvidiaInitialized = nvmlInit() == NVML_SUCCESS;
				if(nvidiaInitialized) [[likely]] {
				
					// Check if getting GPU's UUID as a string was successful
					char uuid[sizeof("GPU-") + HEXADECIMAL_CHARACTER_SIZE * 4 + sizeof('-') + HEXADECIMAL_CHARACTER_SIZE * 2 + sizeof('-') + HEXADECIMAL_CHARACTER_SIZE * 2 + sizeof('-') + HEXADECIMAL_CHARACTER_SIZE * 2 + sizeof('-') + HEXADECIMAL_CHARACTER_SIZE * 6];
					if(sprintf(uuid, "GPU-%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", gpuUuid[0], gpuUuid[1], gpuUuid[2], gpuUuid[3], gpuUuid[4], gpuUuid[5], gpuUuid[6], gpuUuid[7], gpuUuid[8], gpuUuid[9], gpuUuid[10], gpuUuid[11], gpuUuid[12], gpuUuid[13], gpuUuid[14], gpuUuid[15]) == sizeof(uuid) - sizeof('\0')) [[likely]] {
					
						// Check if getting the number of NVIDIA GPUs was successful
						unsigned int numberOfNvidiaGpus;
						if(nvmlDeviceGetCount(&numberOfNvidiaGpus) == NVML_SUCCESS) [[likely]] {
						
							// Go through all NVIDIA GPUs
							for(unsigned int i = 0; i < numberOfNvidiaGpus; ++i) [[likely]] {
							
								// Check if getting the NVIDIA GPU was successful
								if(nvmlDeviceGetHandleByIndex(i, &nvidiaDevice) == NVML_SUCCESS) [[likely]] {
								
									// Check if getting the NVIDIA GPU's UUID was successful and the UUIDs match
									char nvidiaUuid[NVML_DEVICE_UUID_V2_BUFFER_SIZE];
									if(nvmlDeviceGetUUID(nvidiaDevice, nvidiaUuid, sizeof(nvidiaUuid)) == NVML_SUCCESS && !__builtin_strcmp(uuid, nvidiaUuid)) [[unlikely]] {
									
										// Return
										return;
									}
								}
							}
						}
					}
					
					// Shutdown NVIDIA
					nvmlShutdown();
					
					// Set that NVIDIA isn't initialized
					nvidiaInitialized = false;
				}
			}
		#endif
	}
	
	// Energy consumption get total energy consumption
	__attribute__((always_inline)) inline pair<unsigned long long, unsigned long long> EnergyConsumption::getTotalEnergyConsumption() const noexcept {
	
		// Set GPU and CPU total energy consumption to zero
		unsigned long long gpuTotalEnergyConsumption = 0;
		unsigned long long cpuTotalEnergyConsumption = 0;
		
		// Check if not using an Apple device
		#ifndef __APPLE__
		
			// Check if NVIDIA is initialized
			if(nvidiaInitialized) [[likely]] {
			
				// Check if getting the GPU's total energy consumption was successful
				if(nvmlDeviceGetTotalEnergyConsumption(nvidiaDevice, &gpuTotalEnergyConsumption) == NVML_SUCCESS) [[likely]] {
				
					// Make GPU total energy consumption have the correct units
					gpuTotalEnergyConsumption *= NANOJOULES_IN_A_MILLIJOULE;
				}
			}
		#endif
		
		// Check if Windows
		#ifdef _WIN32
		
			// Check if getting energy meter devices was successful
			const HDEVINFO deviceInformationSet = SetupDiGetClassDevs(&GUID_DEVICE_ENERGY_METER, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
			if(deviceInformationSet != INVALID_HANDLE_VALUE) [[likely]] {
			
				// Automatically free device information set when done
				const unique_ptr<remove_pointer_t<HDEVINFO>, decltype(&SetupDiDestroyDeviceInfoList)> deviceInformationSetUniquePointer(deviceInformationSet, SetupDiDestroyDeviceInfoList);
				
				// Go through all energy meter devices
				SP_DEVICE_INTERFACE_DATA device = {
				
					// Size
					.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA)
				};
				
				for(DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInformationSet, nullptr, &GUID_DEVICE_ENERGY_METER, i, &device); ++i) [[likely]] {
				
					// Check if getting the device's details size was successful
					DWORD deviceDetailsSize;
					if(!SetupDiGetDeviceInterfaceDetail(deviceInformationSet, &device, nullptr, 0, &deviceDetailsSize, nullptr) && GetLastError() == ERROR_INSUFFICIENT_BUFFER && deviceDetailsSize) [[likely]] {
					
						// Check if getting the device's details was successful
						alignas(SP_DEVICE_INTERFACE_DETAIL_DATA) uint8_t deviceDetails[deviceDetailsSize];
						reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(deviceDetails)->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
						
						if(SetupDiGetDeviceInterfaceDetail(deviceInformationSet, &device, reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(deviceDetails), deviceDetailsSize, nullptr, nullptr)) [[likely]] {
						
							// Check if opening device's file was successful
							HANDLE deviceFile = CreateFile(reinterpret_cast<const SP_DEVICE_INTERFACE_DETAIL_DATA *>(deviceDetails)->DevicePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
							if(deviceFile != INVALID_HANDLE_VALUE) [[likely]] {
							
								// Automatically close device's file when done
								const unique_ptr<remove_pointer_t<HANDLE>, decltype(&CloseHandle)> deviceFileUniquePointer(deviceFile, CloseHandle);
								
								// Check if getting device's version was successful
								EMI_VERSION version;
								if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_VERSION, nullptr, 0, &version, sizeof(version), nullptr, nullptr)) [[likely]] {
								
									// Check if version is one
									if(version.EmiVersion == EMI_VERSION_V1) [[likely]] {
									
										// Check if getting device's metadata size was successful
										EMI_METADATA_SIZE metadataSize;
										if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_METADATA_SIZE, nullptr, 0, &metadataSize, sizeof(metadataSize), nullptr, nullptr) && metadataSize.MetadataSize) [[likely]] {
										
											// Check if getting device's metadata was successful and device is a CPU socket
											alignas(EMI_METADATA_V1) uint8_t metadata[metadataSize.MetadataSize];
											if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_METADATA, nullptr, 0, metadata, metadataSize.MetadataSize, nullptr, nullptr) && wcslen(reinterpret_cast<const EMI_METADATA_V1 *>(metadata)->MeteredHardwareName) >= sizeof("_PKG") - sizeof('\0') && !_wcsnicmp(&reinterpret_cast<const EMI_METADATA_V1 *>(metadata)->MeteredHardwareName[wcslen(reinterpret_cast<const EMI_METADATA_V1 *>(metadata)->MeteredHardwareName) - (sizeof("_PKG") - sizeof('\0'))], L"_PKG", sizeof("_PKG") - sizeof('\0'))) [[unlikely]] {
											
												// Check if device's measurement unit is compatible
												if(reinterpret_cast<const EMI_METADATA_V1 *>(metadata)->MeasurementUnit == EmiMeasurementUnitPicowattHours) [[likely]] {
												
													// Check if getting device's measurement was successful
													EMI_MEASUREMENT_DATA_V1 measurement;
													if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_MEASUREMENT, nullptr, 0, &measurement, sizeof(measurement), nullptr, nullptr)) [[likely]] {
													
														// Add channel's measurement to the CPU total energy consumption
														cpuTotalEnergyConsumption += measurement.AbsoluteEnergy * PICOJOULES_IN_A_PICOWATT_HOUR;
													}
												}
											}
										}
									}
									
									// Otherwise check if version is two
									else if(version.EmiVersion == EMI_VERSION_V2) [[likely]] {
									
										// Check if getting device's metadata size was successful
										EMI_METADATA_SIZE metadataSize;
										if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_METADATA_SIZE, nullptr, 0, &metadataSize, sizeof(metadataSize), nullptr, nullptr) && metadataSize.MetadataSize) [[likely]] {
										
											// Check if getting device's metadata was successful
											alignas(EMI_METADATA_V2) uint8_t metadata[metadataSize.MetadataSize];
											if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_METADATA, nullptr, 0, metadata, metadataSize.MetadataSize, nullptr, nullptr) && reinterpret_cast<const EMI_METADATA_V2 *>(metadata)->ChannelCount) [[likely]] {
											
												// Check if getting device's measurement was successful
												EMI_MEASUREMENT_DATA_V2 measurement[reinterpret_cast<const EMI_METADATA_V2 *>(metadata)->ChannelCount];
												if(DeviceIoControl(deviceFile, IOCTL_EMI_GET_MEASUREMENT, nullptr, 0, &measurement, sizeof(measurement), nullptr, nullptr)) [[likely]] {
												
													// Go through all of the device's channels
													const EMI_CHANNEL_V2 *channel = &reinterpret_cast<const EMI_METADATA_V2 *>(metadata)->Channels[0];
													for(USHORT j = 0; j < reinterpret_cast<const EMI_METADATA_V2 *>(metadata)->ChannelCount; ++j, channel = EMI_CHANNEL_V2_NEXT_CHANNEL(channel)) [[likely]] {
													
														// Check if channel's measurement unit is compatible and channel is a CPU socket
														if(channel->MeasurementUnit == EmiMeasurementUnitPicowattHours && wcslen(channel->ChannelName) >= sizeof("_PKG") - sizeof('\0') && !_wcsnicmp(&channel->ChannelName[wcslen(channel->ChannelName) - (sizeof("_PKG") - sizeof('\0'))], L"_PKG", sizeof("_PKG") - sizeof('\0'))) [[unlikely]] {
														
															// Add channel's measurement to the CPU total energy consumption
															cpuTotalEnergyConsumption += measurement[0].ChannelData[j].AbsoluteEnergy * PICOJOULES_IN_A_PICOWATT_HOUR;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			
			// Make CPU total energy consumption have the correct units
			cpuTotalEnergyConsumption /= PICOJOULES_IN_A_NANOJOULE;
			
		// Otherwise check if using an Apple device
		#elif defined __APPLE__
		
			// Check if getting samples from the subscription was successful
			const unique_ptr<remove_pointer_t<CFDictionaryRef>, decltype(&CFRelease)> samples(IOReportCreateSamples(subscription.get(), channels.get(), nullptr), CFRelease);
			if(samples) [[likely]] {
			
				// Check if samples are valid
				if(CFGetTypeID(samples.get()) == CFDictionaryGetTypeID()) [[likely]] {
				
					// Check if getting items from the samples was successful
					const CFArrayRef items = reinterpret_cast<CFArrayRef>(CFDictionaryGetValue(samples.get(), CFSTR("IOReportChannels")));
					if(items && CFGetTypeID(items) == CFArrayGetTypeID()) [[likely]] {
					
						// Go through all items
						for(CFIndex i = 0, j = CFArrayGetCount(items); i < j; ++i) [[likely]] {
						
							// Check if getting item was successful
							const CFTypeRef item = CFArrayGetValueAtIndex(items, i);
							if(item) [[likely]] {
							
								// Check if getting item's channel name, unit label, and value was successful
								const CFStringRef channelName = IOReportChannelGetChannelName(item);
								const CFStringRef unitLabel = IOReportChannelGetUnitLabel(item);
								const int64_t value = IOReportSimpleGetIntegerValue(item, 0);
								
								if(channelName && CFGetTypeID(channelName) == CFStringGetTypeID() && unitLabel && CFGetTypeID(unitLabel) == CFStringGetTypeID() && value > 0) [[likely]] {
								
									// Check if channel is desired
									if(
									
										// GPU energy
										CFStringHasSuffix(channelName, CFSTR("GPU Energy")) ||
										
										// CPU energy
										CFStringHasSuffix(channelName, CFSTR("CPU Energy"))
										
									) [[unlikely]] {
									
										// Check if item's units is joules
										unsigned long long valueInCorrectUnits;
										if(CFStringCompare(unitLabel, CFSTR("J"), 0) == kCFCompareEqualTo) [[likely]] {
										
											// Set value in correct units
											valueInCorrectUnits = static_cast<unsigned long long>(value) * NANOJOULES_IN_A_JOULE;
										}
										
										// Otherwise check if item's units is millijoules
										else if(CFStringCompare(unitLabel, CFSTR("mJ"), 0) == kCFCompareEqualTo) [[likely]] {
										
											// Set value in correct units
											valueInCorrectUnits = static_cast<unsigned long long>(value) * NANOJOULES_IN_A_MILLIJOULE;
										}
										
										// Otherwise check if item's units is microjoules
										else if(CFStringCompare(unitLabel, CFSTR("uJ"), 0) == kCFCompareEqualTo) [[likely]] {
										
											// Set value in correct units
											valueInCorrectUnits = static_cast<unsigned long long>(value) * NANOJOULES_IN_A_MICROJOULE;
										}
										
										// Otherwise check if item's units is nanojoules
										else if(CFStringCompare(unitLabel, CFSTR("nJ"), 0) == kCFCompareEqualTo) [[likely]] {
										
											// Set value in correct units
											valueInCorrectUnits = value;
										}
										
										// Otherwise
										else [[unlikely]] {
										
											// Go to next item
											continue;
										}
										
										// Check if item is GPU related
										if(CFStringFind(channelName, CFSTR("GPU"), kCFCompareCaseInsensitive).location != kCFNotFound) [[unlikely]] {
										
											// Add value in correct units to the GPU total energy consumption
											gpuTotalEnergyConsumption += valueInCorrectUnits;
										}
										
										// Otherwise
										else [[likely]] {
										
											// Add value in correct units to the CPU total energy consumption
											cpuTotalEnergyConsumption += valueInCorrectUnits;
										}
									}
								}
							}
						}
					}
				}
			}
			
		// Otherwise
		#else
		
			// Check if opening power capping directory was successful
			const unique_ptr<DIR, decltype(&closedir)> powerCappingDirectory(opendir("/sys/devices/virtual/powercap"), closedir);
			if(powerCappingDirectory) [[likely]] {
			
				// Go through all entries in the power capping directory
				dirent *entry;
				while((entry = readdir(powerCappingDirectory.get()))) [[likely]] {
				
					// Check if entry is a control type
					if(entry->d_type == DT_DIR && entry->d_name[0] && __builtin_strcmp(entry->d_name, ".") && __builtin_strcmp(entry->d_name, "..")) [[likely]] {
					
						// Create control type enabled file path
						char controlTypeEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name) + sizeof("/enabled")];
						__builtin_memcpy(controlTypeEnabledFilePath, "/sys/devices/virtual/powercap/", sizeof("/sys/devices/virtual/powercap/") - sizeof('\0'));
						__builtin_memcpy(&controlTypeEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0')], entry->d_name, __builtin_strlen(entry->d_name));
						__builtin_memcpy(&controlTypeEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name)], "/enabled", sizeof("/enabled"));
						
						// Check if opening control type enabled file was successful and the control type is enabled
						const unique_ptr<FILE, decltype(&fclose)> controlTypeEnabledFile(fopen(controlTypeEnabledFilePath, "rb"), fclose);
						if(controlTypeEnabledFile && fgetc(controlTypeEnabledFile.get()) == '1') [[likely]] {
						
							// Check if opening control type directory was successful
							controlTypeEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name)] = '\0';
							const unique_ptr<DIR, decltype(&closedir)> controlTypeDirectory(opendir(controlTypeEnabledFilePath), closedir);
							if(controlTypeDirectory) [[likely]] {
							
								// Go through all entries in the control type directory
								dirent *controlTypeEntry;
								while((controlTypeEntry = readdir(controlTypeDirectory.get()))) [[likely]] {
								
									// Check if entry is a power zone
									if(controlTypeEntry->d_type == DT_DIR && controlTypeEntry->d_name[0] && __builtin_strcmp(controlTypeEntry->d_name, ".") && __builtin_strcmp(controlTypeEntry->d_name, "..")) [[likely]] {
									
										// Create power zone enabled file path
										char powerZoneEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name) + sizeof('/') + __builtin_strlen(controlTypeEntry->d_name) + sizeof("/energy_uj")];
										__builtin_memcpy(powerZoneEnabledFilePath, controlTypeEnabledFilePath, sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name));
										powerZoneEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name)] = '/';
										__builtin_memcpy(&powerZoneEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name) + sizeof('/')], controlTypeEntry->d_name, __builtin_strlen(controlTypeEntry->d_name));
										__builtin_memcpy(&powerZoneEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name) + sizeof('/') + __builtin_strlen(controlTypeEntry->d_name)], "/enabled", sizeof("/enabled"));
										
										// Check if opening power zone enabled file was successful and the power zone is enabled
										const unique_ptr<FILE, decltype(&fclose)> powerZoneEnabledFile(fopen(powerZoneEnabledFilePath, "rb"), fclose);
										if(powerZoneEnabledFile && fgetc(powerZoneEnabledFile.get()) == '1') [[likely]] {
										
											// Check if opening power zone name file was successful
											__builtin_memcpy(&powerZoneEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name) + sizeof('/') + __builtin_strlen(controlTypeEntry->d_name)], "/name", sizeof("/name"));
											const unique_ptr<FILE, decltype(&fclose)> powerZoneNameFile(fopen(powerZoneEnabledFilePath, "rb"), fclose);
											if(powerZoneNameFile) [[likely]] {
											
												// Check if going to the end of the power zone name file was successful
												while(fgetc(powerZoneNameFile.get()) != EOF) [[likely]];
												
												if(!ferror(powerZoneNameFile.get())) [[likely]] {
												
													// Check if getting power zone name file's size was successful
													const long powerZoneNameSize = ftell(powerZoneNameFile.get());
													if(powerZoneNameSize != -1 && !fseek(powerZoneNameFile.get(), 0, SEEK_SET) && static_cast<unsigned long>(powerZoneNameSize) < SIZE_MAX) [[likely]] {
													
														// Check if getting power zone's name was successful
														char powerZoneName[powerZoneNameSize + sizeof('\0')];
														if(fread(powerZoneName, sizeof(char), powerZoneNameSize, powerZoneNameFile.get()) == static_cast<size_t>(powerZoneNameSize)) [[likely]] {
														
															// Make power zone's name a string
															powerZoneName[powerZoneNameSize] = '\0';
															
															// Check if power zone is a CPU socket
															if(strcasestr(powerZoneName, "package")) [[unlikely]] {
															
																// Check if opening power zone energy file was successful
																__builtin_memcpy(&powerZoneEnabledFilePath[sizeof("/sys/devices/virtual/powercap/") - sizeof('\0') + __builtin_strlen(entry->d_name) + sizeof('/') + __builtin_strlen(controlTypeEntry->d_name)], "/energy_uj", sizeof("/energy_uj"));
																const unique_ptr<FILE, decltype(&fclose)> powerZoneEnergyFile(fopen(powerZoneEnabledFilePath, "rb"), fclose);
																if(powerZoneEnergyFile) [[likely]] {
																
																	// Check if reading the power zone energy file was successful
																	char powerZoneEnergy[MAX_UINT64_STRING_SIZE + sizeof("\n")];
																	const size_t powerZoneEnergySize = fread(powerZoneEnergy, sizeof(char), sizeof(powerZoneEnergy), powerZoneEnergyFile.get());
																	
																	if(powerZoneEnergySize && powerZoneEnergySize <= MAX_UINT64_STRING_SIZE + sizeof('\n') && !__builtin_memchr(powerZoneEnergy, '\0', powerZoneEnergySize)) [[likely]] {
																	
																		// Make power zone energy a string
																		powerZoneEnergy[powerZoneEnergySize - (__builtin_expect(powerZoneEnergy[powerZoneEnergySize - sizeof('\n')] == '\n', true) ? sizeof('\n') : 0)] = '\0';
																		
																		// Check if getting power zone energy as a number was successful
																		char *end;
																		errno = 0;
																		const unsigned long long powerZoneEnergyAsNumber = strtoull(powerZoneEnergy, &end, DECIMAL_NUMBER_BASE);
																		if(end != powerZoneEnergy && !*end && isdigit(powerZoneEnergy[0]) && (powerZoneEnergy[0] != '0' || !isdigit(powerZoneEnergy[sizeof('0')])) && !errno) [[likely]] {
																		
																			// Add power zone energy as a number in correct units to the CPU total energy consumption
																			cpuTotalEnergyConsumption += powerZoneEnergyAsNumber * NANOJOULES_IN_A_MICROJOULE;
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		#endif
		
		// Return GPU and CPU total energy consumption
		return make_pair(gpuTotalEnergyConsumption, cpuTotalEnergyConsumption);
	}
#endif

// Ceil as uint32
__attribute__((always_inline)) static inline constexpr uint32_t ceilAsUint32(const double value) noexcept {

	// Throw error if value is invalid
	assert(("Value is invalid", value >= 0 && value < UINT32_MAX));
	
	// Return one more than the value as an integer if it's less than the value
	const uint32_t integerValue = value;
	return __builtin_expect(integerValue < value, true) ? integerValue + 1 : integerValue;
}

// Ceil as uint64
__attribute__((always_inline)) static inline constexpr uint64_t ceilAsUint64(const double value) noexcept {

	// Throw error if value is invalid
	assert(("Value is invalid", value >= 0 && value < static_cast<double>(UINT64_MAX)));
	
	// Return one more than the value as an integer if it's less than the value
	const uint64_t integerValue = value;
	return __builtin_expect(integerValue < value, true) ? integerValue + 1 : integerValue;
}

// Additional space tolerance percent
__attribute__((always_inline)) static inline constexpr double additionalSpaceTolerancePercent(const unsigned int edgeBits) noexcept {

	// Return additional space tolerance percent
	return 1 + MINIMUM_ADDITIONAL_SPACE_TOLERANCE_PERCENT + (__builtin_expect(edgeBits < MAX_EDGE_BITS, true) ? ((__builtin_expect(edgeBits < MAX_EDGE_BITS - 1, true) ? 1 : 0) + (static_cast<uint32_t>(1) << (MAX_EDGE_BITS - edgeBits - 1))) * ADDITIONAL_SPACE_TOLERANCE_SCALE_FACTOR : 0);
}

// Max number of edges remaining after trimming rounds
__attribute__((always_inline)) static inline constexpr uint64_t maxNumberOfEdgesRemainingAfterTimmingRounds(const unsigned int numberOfTrimmingRounds, const bool additionalSpaceToleranceBasedOnInitialNumberOfEdges) noexcept {

	// Set percent of edges remaining after number of trimming rounds
	const double percentOfEdgesRemainingAfterNumberOfTrimmingRounds[] = {
	
		// After 0 trimming rounds
		1,
		
		// After 1 trimming round
		PERCENT_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND,
		
		// After 2 trimming rounds
		0.296206,
		
		// After 3 trimming rounds
		0.175309,
		
		// After 4 trimming rounds
		0.116781,
		
		// After 5 trimming rounds
		0.0836954,
		
		// After 6 trimming rounds
		0.0630733,
		
		// After 7 trimming rounds
		0.049309,
		
		// After 8 trimming rounds
		0.0396463,
		
		// After 9 trimming rounds
		0.0325937,
		
		// After 10 trimming rounds
		0.027287,
		
		// After 11 trimming rounds
		0.0231887,
		
		// After 12 trimming rounds
		0.0199561,
		
		// After 13 trimming rounds
		0.01736,
		
		// After 14 trimming rounds
		0.0152439,
		
		// After 15 trimming rounds
		0.0134944,
		
		// After 16 trimming rounds
		0.0120316,
		
		// After 17 trimming rounds
		0.0107962,
		
		// After 18 trimming rounds
		0.00974263,
		
		// After 19 trimming rounds
		0.00883723,
		
		// After 20 trimming rounds
		0.00805313,
		
		// After 21 trimming rounds
		0.00736984,
		
		// After 22 trimming rounds
		0.00677025,
		
		// After 23 trimming rounds
		0.00624131,
		
		// After 24 trimming rounds
		0.00577226,
		
		// After 25 trimming rounds
		0.00535436,
		
		// After 26 trimming rounds
		0.00498071,
		
		// After 27 trimming rounds
		0.00464516,
		
		// After 28 trimming rounds
		0.00434256,
		
		// After 29 trimming rounds
		0.00406875,
		
		// After 30 trimming rounds
		0.0038204,
		
		// After 31 trimming rounds
		0.0035942,
		
		// After 32 trimming rounds
		0.00338767,
		
		// After 33 trimming rounds
		0.00319866,
		
		// After 34 trimming rounds
		0.00302515,
		
		// After 35 trimming rounds
		0.00286542,
		
		// After 36 trimming rounds
		0.00271807,
		
		// After 37 trimming rounds
		0.00258179,
		
		// After 38 trimming rounds
		0.00245558,
		
		// After 39 trimming rounds
		0.00233845,
		
		// After 40 trimming rounds
		0.00222948,
		
		// After 41 trimming rounds
		0.00212804,
		
		// After 42 trimming rounds
		0.00203325,
		
		// After 43 trimming rounds
		0.00194471,
		
		// After 44 trimming rounds
		0.00186178,
		
		// After 45 trimming rounds
		0.00178405,
		
		// After 46 trimming rounds
		0.00171109,
		
		// After 47 trimming rounds
		0.00164247,
		
		// After 48 trimming rounds
		0.00157786,
		
		// After 49 trimming rounds
		0.001517,
		
		// After 50 trimming rounds
		0.00145964,
		
		// After 51 trimming rounds
		0.00140545,
		
		// After 52 trimming rounds
		0.00135424,
		
		// After 53 trimming rounds
		0.0013058,
		
		// After 54 trimming rounds
		0.00125992,
		
		// After 55 trimming rounds
		0.00121643,
		
		// After 56 trimming rounds
		0.00117519,
		
		// After 57 trimming rounds
		0.00113602,
		
		// After 58 trimming rounds
		0.00109878,
		
		// After 59 trimming rounds
		0.00106331,
		
		// After 60 trimming rounds
		0.0010297,
		
		// After 61 trimming rounds
		0.000997641,
		
		// After 62 trimming rounds
		0.000967101,
		
		// After 63 trimming rounds
		0.000937999,
		
		// After 64 trimming rounds
		0.000910186,
		
		// After 65 trimming rounds
		0.000883639,
		
		// After 66 trimming rounds
		0.000858259,
		
		// After 67 trimming rounds
		0.000833968,
		
		// After 68 trimming rounds
		0.000810708,
		
		// After 69 trimming rounds
		0.000788403,
		
		// After 70 trimming rounds
		0.000767012,
		
		// After 71 trimming rounds
		0.000746475,
		
		// After 72 trimming rounds
		0.000726751,
		
		// After 73 trimming rounds
		0.000707832,
		
		// After 74 trimming rounds
		0.000689656,
		
		// After 75 trimming rounds
		0.000672136,
		
		// After 76 trimming rounds
		0.00065526,
		
		// After 77 trimming rounds
		0.000638996,
		
		// After 78 trimming rounds
		0.000623332,
		
		// After 79 trimming rounds
		0.000608212,
		
		// After 80 trimming rounds
		0.000593623,
		
		// After 81 trimming rounds
		0.00057953,
		
		// After 82 trimming rounds
		0.000565938,
		
		// After 83 trimming rounds
		0.000552821,
		
		// After 84 trimming rounds
		0.000540158,
		
		// After 85 trimming rounds
		0.000527964,
		
		// After 86 trimming rounds
		0.000516183,
		
		// After 87 trimming rounds
		0.000504809,
		
		// After 88 trimming rounds
		0.000493813,
		
		// After 89 trimming rounds
		0.000483199,
		
		// After 90 trimming rounds
		0.000472921,
		
		// After 91 trimming rounds
		0.00046297,
		
		// After 92 trimming rounds
		0.000453328,
		
		// After 93 trimming rounds
		0.000444001,
		
		// After 94 trimming rounds
		0.000434956,
		
		// After 95 trimming rounds
		0.000426193,
		
		// After 96 trimming rounds
		0.00041769,
		
		// After 97 trimming rounds
		0.000409437,
		
		// After 98 trimming rounds
		0.000401441,
		
		// After 99 trimming rounds
		0.000393684,
		
		// After 100 trimming rounds
		0.000386161,
		
		// After 101 trimming rounds
		0.000378843,
		
		// After 102 trimming rounds
		0.000371739,
		
		// After 103 trimming rounds
		0.000364829,
		
		// After 104 trimming rounds
		0.000358124,
		
		// After 105 trimming rounds
		0.000351615,
		
		// After 106 trimming rounds
		0.000345277,
		
		// After 107 trimming rounds
		0.000339124,
		
		// After 108 trimming rounds
		0.000333135,
		
		// After 109 trimming rounds
		0.000327311,
		
		// After 110 trimming rounds
		0.000321633,
		
		// After 111 trimming rounds
		0.000316104,
		
		// After 112 trimming rounds
		0.000310729,
		
		// After 113 trimming rounds
		0.000305488,
		
		// After 114 trimming rounds
		0.000300374,
		
		// After 115 trimming rounds
		0.000295397,
		
		// After 116 trimming rounds
		0.000290534,
		
		// After 117 trimming rounds
		0.000285798,
		
		// After 118 trimming rounds
		0.00028117,
		
		// After 119 trimming rounds
		0.000276644,
		
		// After 120 trimming rounds
		0.000272239,
		
		// After 121 trimming rounds
		0.000267991,
		
		// After 122 trimming rounds
		0.000263842,
		
		// After 123 trimming rounds
		0.000259788,
		
		// After 124 trimming rounds
		0.000255828,
		
		// After 125 trimming rounds
		0.000251948,
		
		// After 126 trimming rounds
		0.000248169,
		
		// After 127 trimming rounds
		0.000244482,
		
		// After 128 trimming rounds
		0.000240878,
		
		// After 129 trimming rounds
		0.000237361,
		
		// After 130 trimming rounds
		0.00023392,
		
		// After 131 trimming rounds
		0.000230545,
		
		// After 132 trimming rounds
		0.000227239,
		
		// After 133 trimming rounds
		0.000223999,
		
		// After 134 trimming rounds
		0.00022082,
		
		// After 135 trimming rounds
		0.000217711,
		
		// After 136 trimming rounds
		0.000214671,
		
		// After 137 trimming rounds
		0.000211697,
		
		// After 138 trimming rounds
		0.00020879,
		
		// After 139 trimming rounds
		0.000205936,
		
		// After 140 trimming rounds
		0.000203137,
		
		// After 141 trimming rounds
		0.00020039,
		
		// After 142 trimming rounds
		0.000197703,
		
		// After 143 trimming rounds
		0.000195078,
		
		// After 144 trimming rounds
		0.000192501,
		
		// After 145 trimming rounds
		0.00018998,
		
		// After 146 trimming rounds
		0.000187515,
		
		// After 147 trimming rounds
		0.000185107,
		
		// After 148 trimming rounds
		0.000182736,
		
		// After 149 trimming rounds
		0.000180416,
		
		// After 150 trimming rounds
		0.000178131,
		
		// After 151 trimming rounds
		0.000175884,
		
		// After 152 trimming rounds
		0.000173684,
		
		// After 153 trimming rounds
		0.000171522,
		
		// After 154 trimming rounds
		0.000169409,
		
		// After 155 trimming rounds
		0.000167345,
		
		// After 156 trimming rounds
		0.000165314,
		
		// After 157 trimming rounds
		0.000163315,
		
		// After 158 trimming rounds
		0.000161351,
		
		// After 159 trimming rounds
		0.000159421,
		
		// After 160 trimming rounds
		0.000157519,
		
		// After 161 trimming rounds
		0.000155654,
		
		// After 162 trimming rounds
		0.00015382,
		
		// After 163 trimming rounds
		0.000152027,
		
		// After 164 trimming rounds
		0.000150271,
		
		// After 165 trimming rounds
		0.000148547,
		
		// After 166 trimming rounds
		0.000146849,
		
		// After 167 trimming rounds
		0.000145175,
		
		// After 168 trimming rounds
		0.000143526,
		
		// After 169 trimming rounds
		0.000141903,
		
		// After 170 trimming rounds
		0.000140305,
		
		// After 171 trimming rounds
		0.000138731,
		
		// After 172 trimming rounds
		0.000137184,
		
		// After 173 trimming rounds
		0.000135663,
		
		// After 174 trimming rounds
		0.000134167,
		
		// After 175 trimming rounds
		0.000132695,
		
		// After 176 trimming rounds
		0.000131253,
		
		// After 177 trimming rounds
		0.000129836,
		
		// After 178 trimming rounds
		0.000128445,
		
		// After 179 trimming rounds
		0.000127075,
		
		// After 180 trimming rounds
		0.000125728,
		
		// After 181 trimming rounds
		0.000124422,
		
		// After 182 trimming rounds
		0.000123159,
		
		// After 183 trimming rounds
		0.000121919,
		
		// After 184 trimming rounds
		0.000120698,
		
		// After 185 trimming rounds
		0.000119491,
		
		// After 186 trimming rounds
		0.000118303,
		
		// After 187 trimming rounds
		0.000117134,
		
		// After 188 trimming rounds
		0.000115982,
		
		// After 189 trimming rounds
		0.000114846,
		
		// After 190 trimming rounds
		0.000113727,
		
		// After 191 trimming rounds
		0.000112624,
		
		// After 192 trimming rounds
		0.000111536,
		
		// After 193 trimming rounds
		0.000110461,
		
		// After 194 trimming rounds
		0.000109405,
		
		// After 195 trimming rounds
		0.00010836,
		
		// After 196 trimming rounds
		0.000107326,
		
		// After 197 trimming rounds
		0.000106306,
		
		// After 198 trimming rounds
		0.000105303,
		
		// After 199 trimming rounds
		0.000104317,
		
		// After 200 trimming rounds
		0.000103344,
		
		// After 201 trimming rounds
		0.000102386,
		
		// After 202 trimming rounds
		0.00010144,
		
		// After 203 trimming rounds
		0.00010051,
		
		// After 204 trimming rounds
		0.0000995907,
		
		// After 205 trimming rounds
		0.0000986876,
		
		// After 206 trimming rounds
		0.0000977989,
		
		// After 207 trimming rounds
		0.0000969251,
		
		// After 208 trimming rounds
		0.0000960722,
		
		// After 209 trimming rounds
		0.0000952338,
		
		// After 210 trimming rounds
		0.0000944072,
		
		// After 211 trimming rounds
		0.0000935937,
		
		// After 212 trimming rounds
		0.0000927884,
		
		// After 213 trimming rounds
		0.0000919944,
		
		// After 214 trimming rounds
		0.0000912105,
		
		// After 215 trimming rounds
		0.0000904333,
		
		// After 216 trimming rounds
		0.0000896635,
		
		// After 217 trimming rounds
		0.0000889031,
		
		// After 218 trimming rounds
		0.0000881511,
		
		// After 219 trimming rounds
		0.0000874051,
		
		// After 220 trimming rounds
		0.0000866677,
		
		// After 221 trimming rounds
		0.0000859397,
		
		// After 222 trimming rounds
		0.000085226,
		
		// After 223 trimming rounds
		0.0000845229,
		
		// After 224 trimming rounds
		0.0000838309,
		
		// After 225 trimming rounds
		0.0000831452,
		
		// After 226 trimming rounds
		0.0000824651,
		
		// After 227 trimming rounds
		0.0000817969,
		
		// After 228 trimming rounds
		0.0000811366,
		
		// After 229 trimming rounds
		0.0000804842,
		
		// After 230 trimming rounds
		0.0000798397,
		
		// After 231 trimming rounds
		0.0000792043,
		
		// After 232 trimming rounds
		0.0000785792,
		
		// After 233 trimming rounds
		0.0000779617,
		
		// After 234 trimming rounds
		0.0000773515,
		
		// After 235 trimming rounds
		0.0000767503,
		
		// After 236 trimming rounds
		0.0000761531,
		
		// After 237 trimming rounds
		0.000075565,
		
		// After 238 trimming rounds
		0.0000749843,
		
		// After 239 trimming rounds
		0.0000744108,
		
		// After 240 trimming rounds
		0.0000738427,
		
		// After 241 trimming rounds
		0.0000732783,
		
		// After 242 trimming rounds
		0.000072723,
		
		// After 243 trimming rounds
		0.000072174,
		
		// After 244 trimming rounds
		0.0000716296,
		
		// After 245 trimming rounds
		0.0000710899,
		
		// After 246 trimming rounds
		0.0000705598,
		
		// After 247 trimming rounds
		0.0000700355,
		
		// After 248 trimming rounds
		0.0000695202,
		
		// After 249 trimming rounds
		0.0000690117,
		
		// After 250 trimming rounds
		0.0000685116,
		
		// After 251 trimming rounds
		0.0000680175,
		
		// After 252 trimming rounds
		0.000067529,
		
		// After 253 trimming rounds
		0.0000670461,
		
		// After 254 trimming rounds
		0.0000665714,
		
		// After 255 trimming rounds
		0.0000661004,
		
		// After 256 trimming rounds
		0.000065635,
		
		// After 257 trimming rounds
		0.0000651777,
		
		// After 258 trimming rounds
		0.0000647297,
		
		// After 259 trimming rounds
		0.0000642904,
		
		// After 260 trimming rounds
		0.000063858,
		
		// After 261 trimming rounds
		0.000063431,
		
		// After 262 trimming rounds
		0.000063007,
		
		// After 263 trimming rounds
		0.0000625877,
		
		// After 264 trimming rounds
		0.0000621721,
		
		// After 265 trimming rounds
		0.0000617607,
		
		// After 266 trimming rounds
		0.0000613527,
		
		// After 267 trimming rounds
		0.0000609537,
		
		// After 268 trimming rounds
		0.0000605569,
		
		// After 269 trimming rounds
		0.0000601625,
		
		// After 270 trimming rounds
		0.0000597704,
		
		// After 271 trimming rounds
		0.0000593821,
		
		// After 272 trimming rounds
		0.0000589991,
		
		// After 273 trimming rounds
		0.0000586191,
		
		// After 274 trimming rounds
		0.0000582424,
		
		// After 275 trimming rounds
		0.0000578696,
		
		// After 276 trimming rounds
		0.0000575008,
		
		// After 277 trimming rounds
		0.0000571348,
		
		// After 278 trimming rounds
		0.0000567725,
		
		// After 279 trimming rounds
		0.0000564149,
		
		// After 280 trimming rounds
		0.0000560619,
		
		// After 281 trimming rounds
		0.0000557126,
		
		// After 282 trimming rounds
		0.0000553681,
		
		// After 283 trimming rounds
		0.0000550242,
		
		// After 284 trimming rounds
		0.0000546845,
		
		// After 285 trimming rounds
		0.0000543471,
		
		// After 286 trimming rounds
		0.0000540134,
		
		// After 287 trimming rounds
		0.0000536835,
		
		// After 288 trimming rounds
		0.0000533564,
		
		// After 289 trimming rounds
		0.0000530325,
		
		// After 290 trimming rounds
		0.0000527136,
		
		// After 291 trimming rounds
		0.0000523983,
		
		// After 292 trimming rounds
		0.0000520861,
		
		// After 293 trimming rounds
		0.0000517771,
		
		// After 294 trimming rounds
		0.0000514702,
		
		// After 295 trimming rounds
		0.0000511666,
		
		// After 296 trimming rounds
		0.0000508677,
		
		// After 297 trimming rounds
		0.0000505724,
		
		// After 298 trimming rounds
		0.0000502786,
		
		// After 299 trimming rounds
		0.0000499878,
		
		// After 300 trimming rounds
		0.0000496984,
		
		// After 301 trimming rounds
		0.0000494104,
		
		// After 302 trimming rounds
		0.0000491263,
		
		// After 303 trimming rounds
		0.0000488455,
		
		// After 304 trimming rounds
		0.0000485666,
		
		// After 305 trimming rounds
		0.0000482891,
		
		// After 306 trimming rounds
		0.0000480169,
		
		// After 307 trimming rounds
		0.0000477459,
		
		// After 308 trimming rounds
		0.0000474772,
		
		// After 309 trimming rounds
		0.0000472115,
		
		// After 310 trimming rounds
		0.0000469482,
		
		// After 311 trimming rounds
		0.0000466856,
		
		// After 312 trimming rounds
		0.0000464239,
		
		// After 313 trimming rounds
		0.000046165,
		
		// After 314 trimming rounds
		0.0000459093,
		
		// After 315 trimming rounds
		0.0000456565,
		
		// After 316 trimming rounds
		0.0000454045,
		
		// After 317 trimming rounds
		0.0000451549,
		
		// After 318 trimming rounds
		0.0000449091,
		
		// After 319 trimming rounds
		0.0000446667,
		
		// After 320 trimming rounds
		0.000044425,
		
		// After 321 trimming rounds
		0.0000441847,
		
		// After 322 trimming rounds
		0.000043944,
		
		// After 323 trimming rounds
		0.0000437065,
		
		// After 324 trimming rounds
		0.0000434723,
		
		// After 325 trimming rounds
		0.0000432397,
		
		// After 326 trimming rounds
		0.0000430085,
		
		// After 327 trimming rounds
		0.0000427787,
		
		// After 328 trimming rounds
		0.0000425496,
		
		// After 329 trimming rounds
		0.0000423221,
		
		// After 330 trimming rounds
		0.0000420944,
		
		// After 331 trimming rounds
		0.0000418685,
		
		// After 332 trimming rounds
		0.0000416448,
		
		// After 333 trimming rounds
		0.000041422,
		
		// After 334 trimming rounds
		0.0000412022,
		
		// After 335 trimming rounds
		0.0000409833,
		
		// After 336 trimming rounds
		0.0000407686,
		
		// After 337 trimming rounds
		0.0000405554,
		
		// After 338 trimming rounds
		0.000040344,
		
		// After 339 trimming rounds
		0.000040133,
		
		// After 340 trimming rounds
		0.0000399239,
		
		// After 341 trimming rounds
		0.0000397156,
		
		// After 342 trimming rounds
		0.0000395081,
		
		// After 343 trimming rounds
		0.000039302,
		
		// After 344 trimming rounds
		0.0000390986,
		
		// After 345 trimming rounds
		0.0000388983,
		
		// After 346 trimming rounds
		0.0000387004,
		
		// After 347 trimming rounds
		0.0000385055,
		
		// After 348 trimming rounds
		0.0000383132,
		
		// After 349 trimming rounds
		0.0000381228,
		
		// After 350 trimming rounds
		0.0000379337,
		
		// After 351 trimming rounds
		0.0000377456,
		
		// After 352 trimming rounds
		0.0000375591,
		
		// After 353 trimming rounds
		0.0000373744,
		
		// After 354 trimming rounds
		0.0000371924,
		
		// After 355 trimming rounds
		0.0000370117,
		
		// After 356 trimming rounds
		0.0000368317,
		
		// After 357 trimming rounds
		0.0000366524,
		
		// After 358 trimming rounds
		0.0000364736,
		
		// After 359 trimming rounds
		0.0000362955,
		
		// After 360 trimming rounds
		0.0000361193,
		
		// After 361 trimming rounds
		0.0000359449,
		
		// After 362 trimming rounds
		0.0000357716,
		
		// After 363 trimming rounds
		0.0000355986,
		
		// After 364 trimming rounds
		0.0000354259,
		
		// After 365 trimming rounds
		0.0000352538,
		
		// After 366 trimming rounds
		0.0000350834,
		
		// After 367 trimming rounds
		0.0000349146,
		
		// After 368 trimming rounds
		0.0000347462,
		
		// After 369 trimming rounds
		0.0000345795,
		
		// After 370 trimming rounds
		0.0000344138,
		
		// After 371 trimming rounds
		0.0000342478,
		
		// After 372 trimming rounds
		0.0000340824,
		
		// After 373 trimming rounds
		0.0000339178,
		
		// After 374 trimming rounds
		0.0000337553,
		
		// After 375 trimming rounds
		0.000033594,
		
		// After 376 trimming rounds
		0.0000334338,
		
		// After 377 trimming rounds
		0.0000332745,
		
		// After 378 trimming rounds
		0.0000331164,
		
		// After 379 trimming rounds
		0.00003296,
		
		// After 380 trimming rounds
		0.0000328044,
		
		// After 381 trimming rounds
		0.0000326501,
		
		// After 382 trimming rounds
		0.0000324966,
		
		// After 383 trimming rounds
		0.0000323458,
		
		// After 384 trimming rounds
		0.0000321963,
		
		// After 385 trimming rounds
		0.0000320473,
		
		// After 386 trimming rounds
		0.0000318999,
		
		// After 387 trimming rounds
		0.0000317539,
		
		// After 388 trimming rounds
		0.0000316096,
		
		// After 389 trimming rounds
		0.0000314678,
		
		// After 390 trimming rounds
		0.0000313278,
		
		// After 391 trimming rounds
		0.0000311886,
		
		// After 392 trimming rounds
		0.0000310491,
		
		// After 393 trimming rounds
		0.0000309111,
		
		// After 394 trimming rounds
		0.0000307737,
		
		// After 395 trimming rounds
		0.0000306377,
		
		// After 396 trimming rounds
		0.0000305022,
		
		// After 397 trimming rounds
		0.0000303674,
		
		// After 398 trimming rounds
		0.0000302331,
		
		// After 399 trimming rounds
		0.0000300996,
		
		// After 400 trimming rounds
		0.0000299679,
		
		// After 401 trimming rounds
		0.0000298368,
		
		// After 402 trimming rounds
		0.0000297066,
		
		// After 403 trimming rounds
		0.0000295765,
		
		// After 404 trimming rounds
		0.0000294463,
		
		// After 405 trimming rounds
		0.0000293178,
		
		// After 406 trimming rounds
		0.00002919,
		
		// After 407 trimming rounds
		0.000029064,
		
		// After 408 trimming rounds
		0.0000289392,
		
		// After 409 trimming rounds
		0.0000288154,
		
		// After 410 trimming rounds
		0.0000286913,
		
		// After 411 trimming rounds
		0.0000285674,
		
		// After 412 trimming rounds
		0.0000284442,
		
		// After 413 trimming rounds
		0.0000283211,
		
		// After 414 trimming rounds
		0.0000281981,
		
		// After 415 trimming rounds
		0.0000280757,
		
		// After 416 trimming rounds
		0.0000279536,
		
		// After 417 trimming rounds
		0.0000278321,
		
		// After 418 trimming rounds
		0.0000277115,
		
		// After 419 trimming rounds
		0.0000275909,
		
		// After 420 trimming rounds
		0.000027481,
		
		// After 421 trimming rounds
		0.0000273727,
		
		// After 422 trimming rounds
		0.0000272649,
		
		// After 423 trimming rounds
		0.0000271569,
		
		// After 424 trimming rounds
		0.0000270496,
		
		// After 425 trimming rounds
		0.0000269432,
		
		// After 426 trimming rounds
		0.0000268365,
		
		// After 427 trimming rounds
		0.0000267301,
		
		// After 428 trimming rounds
		0.000026624,
		
		// After 429 trimming rounds
		0.0000265192,
		
		// After 430 trimming rounds
		0.0000264151,
		
		// After 431 trimming rounds
		0.0000263117,
		
		// After 432 trimming rounds
		0.0000262083,
		
		// After 433 trimming rounds
		0.000026105,
		
		// After 434 trimming rounds
		0.0000260018,
		
		// After 435 trimming rounds
		0.0000258994,
		
		// After 436 trimming rounds
		0.0000257981,
		
		// After 437 trimming rounds
		0.0000256973,
		
		// After 438 trimming rounds
		0.000025596,
		
		// After 439 trimming rounds
		0.0000254961,
		
		// After 440 trimming rounds
		0.0000253967,
		
		// After 441 trimming rounds
		0.0000252982,
		
		// After 442 trimming rounds
		0.0000252004,
		
		// After 443 trimming rounds
		0.0000251038,
		
		// After 444 trimming rounds
		0.0000250076,
		
		// After 445 trimming rounds
		0.0000249124,
		
		// After 446 trimming rounds
		0.0000248181,
		
		// After 447 trimming rounds
		0.0000247241,
		
		// After 448 trimming rounds
		0.0000246314,
		
		// After 449 trimming rounds
		0.0000245403,
		
		// After 450 trimming rounds
		0.0000244502,
		
		// After 451 trimming rounds
		0.0000243608,
		
		// After 452 trimming rounds
		0.0000242735,
		
		// After 453 trimming rounds
		0.0000241864,
		
		// After 454 trimming rounds
		0.0000240998,
		
		// After 455 trimming rounds
		0.0000240135,
		
		// After 456 trimming rounds
		0.0000239266,
		
		// After 457 trimming rounds
		0.0000238395,
		
		// After 458 trimming rounds
		0.0000237536,
		
		// After 459 trimming rounds
		0.0000236691,
		
		// After 460 trimming rounds
		0.0000235848,
		
		// After 461 trimming rounds
		0.0000235003,
		
		// After 462 trimming rounds
		0.0000234167,
		
		// After 463 trimming rounds
		0.0000233338,
		
		// After 464 trimming rounds
		0.0000232519,
		
		// After 465 trimming rounds
		0.0000231706,
		
		// After 466 trimming rounds
		0.00002309,
		
		// After 467 trimming rounds
		0.0000230102,
		
		// After 468 trimming rounds
		0.000022931,
		
		// After 469 trimming rounds
		0.0000228519,
		
		// After 470 trimming rounds
		0.0000227732,
		
		// After 471 trimming rounds
		0.0000226942,
		
		// After 472 trimming rounds
		0.0000226155,
		
		// After 473 trimming rounds
		0.0000225364,
		
		// After 474 trimming rounds
		0.000022457,
		
		// After 475 trimming rounds
		0.000022379,
		
		// After 476 trimming rounds
		0.0000223012,
		
		// After 477 trimming rounds
		0.0000222248,
		
		// After 478 trimming rounds
		0.0000221492,
		
		// After 479 trimming rounds
		0.000022074,
		
		// After 480 trimming rounds
		0.0000219995,
		
		// After 481 trimming rounds
		0.000021925,
		
		// After 482 trimming rounds
		0.0000218514,
		
		// After 483 trimming rounds
		0.000021778,
		
		// After 484 trimming rounds
		0.0000217056,
		
		// After 485 trimming rounds
		0.0000216342,
		
		// After 486 trimming rounds
		0.0000215627,
		
		// After 487 trimming rounds
		0.0000214928,
		
		// After 488 trimming rounds
		0.0000214234,
		
		// After 489 trimming rounds
		0.0000213545,
		
		// After 490 trimming rounds
		0.0000212856,
		
		// After 491 trimming rounds
		0.0000212167,
		
		// After 492 trimming rounds
		0.0000211482,
		
		// After 493 trimming rounds
		0.0000210798,
		
		// After 494 trimming rounds
		0.0000210116,
		
		// After 495 trimming rounds
		0.0000209436,
		
		// After 496 trimming rounds
		0.0000208761,
		
		// After 497 trimming rounds
		0.0000208083,
		
		// After 498 trimming rounds
		0.000020741,
		
		// After 499 trimming rounds
		0.0000206742,
		
		// After 500 trimming rounds
		0.0000206071,
		
		// After 501 trimming rounds
		0.0000205406,
		
		// After 502 trimming rounds
		0.0000204744,
		
		// After 503 trimming rounds
		0.0000204088,
		
		// After 504 trimming rounds
		0.0000203429,
		
		// After 505 trimming rounds
		0.0000202772,
		
		// After 506 trimming rounds
		0.0000202118,
		
		// After 507 trimming rounds
		0.0000201461,
		
		// After 508 trimming rounds
		0.0000200809,
		
		// After 509 trimming rounds
		0.000020016,
		
		// After 510 trimming rounds
		0.0000199508,
		
		// After 511 trimming rounds
		0.0000198856,
		
		// After 512 trimming rounds
		0.0000198213,
		
		// After 513 trimming rounds
		0.0000197575,
		
		// After 514 trimming rounds
		0.0000196947,
		
		// After 515 trimming rounds
		0.0000196325,
		
		// After 516 trimming rounds
		0.0000195703,
		
		// After 517 trimming rounds
		0.0000195079,
		
		// After 518 trimming rounds
		0.0000194455,
		
		// After 519 trimming rounds
		0.0000193834,
		
		// After 520 trimming rounds
		0.0000193212,
		
		// After 521 trimming rounds
		0.0000192598,
		
		// After 522 trimming rounds
		0.0000191983,
		
		// After 523 trimming rounds
		0.0000191373,
		
		// After 524 trimming rounds
		0.000019077,
		
		// After 525 trimming rounds
		0.0000190174,
		
		// After 526 trimming rounds
		0.000018958,
		
		// After 527 trimming rounds
		0.0000188998,
		
		// After 528 trimming rounds
		0.0000188416,
		
		// After 529 trimming rounds
		0.0000187841,
		
		// After 530 trimming rounds
		0.000018727,
		
		// After 531 trimming rounds
		0.0000186698,
		
		// After 532 trimming rounds
		0.0000186125,
		
		// After 533 trimming rounds
		0.0000185559,
		
		// After 534 trimming rounds
		0.0000184998,
		
		// After 535 trimming rounds
		0.0000184437,
		
		// After 536 trimming rounds
		0.000018389,
		
		// After 537 trimming rounds
		0.0000183342,
		
		// After 538 trimming rounds
		0.00001828,
		
		// After 539 trimming rounds
		0.000018226,
		
		// After 540 trimming rounds
		0.000018172,
		
		// After 541 trimming rounds
		0.0000181179,
		
		// After 542 trimming rounds
		0.0000180646,
		
		// After 543 trimming rounds
		0.0000180113,
		
		// After 544 trimming rounds
		0.0000179582,
		
		// After 545 trimming rounds
		0.0000179054,
		
		// After 546 trimming rounds
		0.0000178528,
		
		// After 547 trimming rounds
		0.0000178006,
		
		// After 548 trimming rounds
		0.0000177484,
		
		// After 549 trimming rounds
		0.0000176963,
		
		// After 550 trimming rounds
		0.0000176441,
		
		// After 551 trimming rounds
		0.0000175927,
		
		// After 552 trimming rounds
		0.0000175417,
		
		// After 553 trimming rounds
		0.0000174905,
		
		// After 554 trimming rounds
		0.0000174395,
		
		// After 555 trimming rounds
		0.000017389,
		
		// After 556 trimming rounds
		0.0000173384,
		
		// After 557 trimming rounds
		0.0000172881,
		
		// After 558 trimming rounds
		0.0000172383,
		
		// After 559 trimming rounds
		0.0000171883,
		
		// After 560 trimming rounds
		0.000017138,
		
		// After 561 trimming rounds
		0.0000170879,
		
		// After 562 trimming rounds
		0.0000170374,
		
		// After 563 trimming rounds
		0.0000169876,
		
		// After 564 trimming rounds
		0.0000169375,
		
		// After 565 trimming rounds
		0.000016887,
		
		// After 566 trimming rounds
		0.0000168369,
		
		// After 567 trimming rounds
		0.0000167866,
		
		// After 568 trimming rounds
		0.0000167361,
		
		// After 569 trimming rounds
		0.000016686,
		
		// After 570 trimming rounds
		0.000016636,
		
		// After 571 trimming rounds
		0.0000165871,
		
		// After 572 trimming rounds
		0.0000165382,
		
		// After 573 trimming rounds
		0.0000164891,
		
		// After 574 trimming rounds
		0.0000164402,
		
		// After 575 trimming rounds
		0.000016392,
		
		// After 576 trimming rounds
		0.000016344,
		
		// After 577 trimming rounds
		0.000016296,
		
		// After 578 trimming rounds
		0.0000162483,
		
		// After 579 trimming rounds
		0.0000162008,
		
		// After 580 trimming rounds
		0.000016154,
		
		// After 581 trimming rounds
		0.0000161079,
		
		// After 582 trimming rounds
		0.0000160621,
		
		// After 583 trimming rounds
		0.0000160167,
		
		// After 584 trimming rounds
		0.0000159713,
		
		// After 585 trimming rounds
		0.0000159261,
		
		// After 586 trimming rounds
		0.0000158811,
		
		// After 587 trimming rounds
		0.000015836,
		
		// After 588 trimming rounds
		0.0000157908,
		
		// After 589 trimming rounds
		0.0000157454,
		
		// After 590 trimming rounds
		0.0000157,
		
		// After 591 trimming rounds
		0.0000156551,
		
		// After 592 trimming rounds
		0.0000156101,
		
		// After 593 trimming rounds
		0.0000155652,
		
		// After 594 trimming rounds
		0.0000155214,
		
		// After 595 trimming rounds
		0.0000154774,
		
		// After 596 trimming rounds
		0.0000154339,
		
		// After 597 trimming rounds
		0.000015391,
		
		// After 598 trimming rounds
		0.0000153482,
		
		// After 599 trimming rounds
		0.0000153054,
		
		// After 600 trimming rounds
		0.0000152637,
		
		// After 601 trimming rounds
		0.0000152218,
		
		// After 602 trimming rounds
		0.0000151799,
		
		// After 603 trimming rounds
		0.0000151379,
		
		// After 604 trimming rounds
		0.0000150965,
		
		// After 605 trimming rounds
		0.0000150551,
		
		// After 606 trimming rounds
		0.0000150136,
		
		// After 607 trimming rounds
		0.0000149724,
		
		// After 608 trimming rounds
		0.0000149328,
		
		// After 609 trimming rounds
		0.0000148937,
		
		// After 610 trimming rounds
		0.0000148555,
		
		// After 611 trimming rounds
		0.0000148173,
		
		// After 612 trimming rounds
		0.0000147794,
		
		// After 613 trimming rounds
		0.0000147417,
		
		// After 614 trimming rounds
		0.0000147044,
		
		// After 615 trimming rounds
		0.0000146672,
		
		// After 616 trimming rounds
		0.0000146301,
		
		// After 617 trimming rounds
		0.0000145934,
		
		// After 618 trimming rounds
		0.0000145563,
		
		// After 619 trimming rounds
		0.0000145196,
		
		// After 620 trimming rounds
		0.0000144825,
		
		// After 621 trimming rounds
		0.0000144523,
		
		// After 622 trimming rounds
		0.0000144225,
		
		// After 623 trimming rounds
		0.0000143927,
		
		// After 624 trimming rounds
		0.0000143633,
		
		// After 625 trimming rounds
		0.0000143345,
		
		// After 626 trimming rounds
		0.0000143056,
		
		// After 627 trimming rounds
		0.0000142774,
		
		// After 628 trimming rounds
		0.0000142492,
		
		// After 629 trimming rounds
		0.0000142215,
		
		// After 630 trimming rounds
		0.0000141938,
		
		// After 631 trimming rounds
		0.0000141663,
		
		// After 632 trimming rounds
		0.0000141389,
		
		// After 633 trimming rounds
		0.0000141116,
		
		// After 634 trimming rounds
		0.0000140844,
		
		// After 635 trimming rounds
		0.0000140574,
		
		// After 636 trimming rounds
		0.0000140308,
		
		// After 637 trimming rounds
		0.0000140041,
		
		// After 638 trimming rounds
		0.0000139773,
		
		// After 639 trimming rounds
		0.000013951,
		
		// After 640 trimming rounds
		0.0000139249,
		
		// After 641 trimming rounds
		0.0000138988,
		
		// After 642 trimming rounds
		0.0000138727,
		
		// After 643 trimming rounds
		0.0000138469,
		
		// After 644 trimming rounds
		0.0000138208,
		
		// After 645 trimming rounds
		0.0000137954,
		
		// After 646 trimming rounds
		0.0000137703,
		
		// After 647 trimming rounds
		0.0000137449,
		
		// After 648 trimming rounds
		0.0000137195,
		
		// After 649 trimming rounds
		0.0000136944,
		
		// After 650 trimming rounds
		0.0000136695,
		
		// After 651 trimming rounds
		0.0000136443,
		
		// After 652 trimming rounds
		0.0000136192,
		
		// After 653 trimming rounds
		0.0000135945,
		
		// After 654 trimming rounds
		0.0000135696,
		
		// After 655 trimming rounds
		0.0000135449,
		
		// After 656 trimming rounds
		0.0000135202,
		
		// After 657 trimming rounds
		0.0000134958,
		
		// After 658 trimming rounds
		0.0000134716,
		
		// After 659 trimming rounds
		0.0000134474,
		
		// After 660 trimming rounds
		0.0000134234,
		
		// After 661 trimming rounds
		0.0000133994,
		
		// After 662 trimming rounds
		0.0000133752,
		
		// After 663 trimming rounds
		0.000013351,
		
		// After 664 trimming rounds
		0.0000133268,
		
		// After 665 trimming rounds
		0.0000133025,
		
		// After 666 trimming rounds
		0.0000132781,
		
		// After 667 trimming rounds
		0.0000132537,
		
		// After 668 trimming rounds
		0.0000132292,
		
		// After 669 trimming rounds
		0.0000132048,
		
		// After 670 trimming rounds
		0.0000131805,
		
		// After 671 trimming rounds
		0.0000131566,
		
		// After 672 trimming rounds
		0.0000131323,
		
		// After 673 trimming rounds
		0.0000131081,
		
		// After 674 trimming rounds
		0.0000130844,
		
		// After 675 trimming rounds
		0.0000130606,
		
		// After 676 trimming rounds
		0.0000130374,
		
		// After 677 trimming rounds
		0.0000130141,
		
		// After 678 trimming rounds
		0.0000129908,
		
		// After 679 trimming rounds
		0.0000129675,
		
		// After 680 trimming rounds
		0.0000129442,
		
		// After 681 trimming rounds
		0.0000129209,
		
		// After 682 trimming rounds
		0.0000128977,
		
		// After 683 trimming rounds
		0.0000128744,
		
		// After 684 trimming rounds
		0.0000128513,
		
		// After 685 trimming rounds
		0.0000128287,
		
		// After 686 trimming rounds
		0.0000128062,
		
		// After 687 trimming rounds
		0.0000127836,
		
		// After 688 trimming rounds
		0.000012761,
		
		// After 689 trimming rounds
		0.0000127386,
		
		// After 690 trimming rounds
		0.0000127163,
		
		// After 691 trimming rounds
		0.0000126939,
		
		// After 692 trimming rounds
		0.0000126716,
		
		// After 693 trimming rounds
		0.0000126492,
		
		// After 694 trimming rounds
		0.0000126273,
		
		// After 695 trimming rounds
		0.0000126055,
		
		// After 696 trimming rounds
		0.0000125836,
		
		// After 697 trimming rounds
		0.0000125617,
		
		// After 698 trimming rounds
		0.0000125398,
		
		// After 699 trimming rounds
		0.0000125181,
		
		// After 700 trimming rounds
		0.0000124965,
		
		// After 701 trimming rounds
		0.0000124751,
		
		// After 702 trimming rounds
		0.0000124539,
		
		// After 703 trimming rounds
		0.0000124329,
		
		// After 704 trimming rounds
		0.000012412,
		
		// After 705 trimming rounds
		0.0000123912,
		
		// After 706 trimming rounds
		0.0000123705,
		
		// After 707 trimming rounds
		0.00001235,
		
		// After 708 trimming rounds
		0.0000123295,
		
		// After 709 trimming rounds
		0.0000123093,
		
		// After 710 trimming rounds
		0.000012289,
		
		// After 711 trimming rounds
		0.0000122685,
		
		// After 712 trimming rounds
		0.000012249,
		
		// After 713 trimming rounds
		0.0000122292,
		
		// After 714 trimming rounds
		0.0000122094,
		
		// After 715 trimming rounds
		0.0000121896,
		
		// After 716 trimming rounds
		0.0000121701,
		
		// After 717 trimming rounds
		0.0000121505,
		
		// After 718 trimming rounds
		0.0000121309,
		
		// After 719 trimming rounds
		0.0000121119,
		
		// After 720 trimming rounds
		0.0000120928,
		
		// After 721 trimming rounds
		0.0000120739,
		
		// After 722 trimming rounds
		0.000012055,
		
		// After 723 trimming rounds
		0.0000120362,
		
		// After 724 trimming rounds
		0.0000120173,
		
		// After 725 trimming rounds
		0.0000119989,
		
		// After 726 trimming rounds
		0.0000119808,
		
		// After 727 trimming rounds
		0.0000119628,
		
		// After 728 trimming rounds
		0.0000119449,
		
		// After 729 trimming rounds
		0.000011927,
		
		// After 730 trimming rounds
		0.0000119091,
		
		// After 731 trimming rounds
		0.0000118911,
		
		// After 732 trimming rounds
		0.0000118732,
		
		// After 733 trimming rounds
		0.0000118553,
		
		// After 734 trimming rounds
		0.0000118373,
		
		// After 735 trimming rounds
		0.0000118194,
		
		// After 736 trimming rounds
		0.0000118015,
		
		// After 737 trimming rounds
		0.0000117836,
		
		// After 738 trimming rounds
		0.0000117656,
		
		// After 739 trimming rounds
		0.0000117477,
		
		// After 740 trimming rounds
		0.0000117298,
		
		// After 741 trimming rounds
		0.0000117121,
		
		// After 742 trimming rounds
		0.0000116949,
		
		// After 743 trimming rounds
		0.0000116779,
		
		// After 744 trimming rounds
		0.0000116609,
		
		// After 745 trimming rounds
		0.0000116436,
		
		// After 746 trimming rounds
		0.0000116264,
		
		// After 747 trimming rounds
		0.0000116092,
		
		// After 748 trimming rounds
		0.0000115919,
		
		// After 749 trimming rounds
		0.0000115747,
		
		// After 750 trimming rounds
		0.0000115575,
		
		// After 751 trimming rounds
		0.00001154,
		
		// After 752 trimming rounds
		0.0000115226,
		
		// After 753 trimming rounds
		0.0000115058,
		
		// After 754 trimming rounds
		0.0000114893,
		
		// After 755 trimming rounds
		0.0000114727,
		
		// After 756 trimming rounds
		0.0000114562,
		
		// After 757 trimming rounds
		0.0000114397,
		
		// After 758 trimming rounds
		0.0000114231,
		
		// After 759 trimming rounds
		0.0000114066,
		
		// After 760 trimming rounds
		0.0000113901,
		
		// After 761 trimming rounds
		0.0000113735,
		
		// After 762 trimming rounds
		0.000011357,
		
		// After 763 trimming rounds
		0.0000113405,
		
		// After 764 trimming rounds
		0.000011324,
		
		// After 765 trimming rounds
		0.0000113074,
		
		// After 766 trimming rounds
		0.0000112909,
		
		// After 767 trimming rounds
		0.0000112744,
		
		// After 768 trimming rounds
		0.0000112578,
		
		// After 769 trimming rounds
		0.0000112413,
		
		// After 770 trimming rounds
		0.0000112252,
		
		// After 771 trimming rounds
		0.0000112089,
		
		// After 772 trimming rounds
		0.0000111926,
		
		// After 773 trimming rounds
		0.0000111763,
		
		// After 774 trimming rounds
		0.00001116,
		
		// After 775 trimming rounds
		0.0000111442,
		
		// After 776 trimming rounds
		0.0000111284,
		
		// After 777 trimming rounds
		0.0000111128,
		
		// After 778 trimming rounds
		0.0000110972,
		
		// After 779 trimming rounds
		0.0000110816,
		
		// After 780 trimming rounds
		0.000011066,
		
		// After 781 trimming rounds
		0.0000110504,
		
		// After 782 trimming rounds
		0.0000110348,
		
		// After 783 trimming rounds
		0.0000110194,
		
		// After 784 trimming rounds
		0.000011004,
		
		// After 785 trimming rounds
		0.0000109887,
		
		// After 786 trimming rounds
		0.0000109731,
		
		// After 787 trimming rounds
		0.0000109575,
		
		// After 788 trimming rounds
		0.0000109419,
		
		// After 789 trimming rounds
		0.0000109263,
		
		// After 790 trimming rounds
		0.0000109107,
		
		// After 791 trimming rounds
		0.0000108951,
		
		// After 792 trimming rounds
		0.0000108795,
		
		// After 793 trimming rounds
		0.0000108639,
		
		// After 794 trimming rounds
		0.0000108492,
		
		// After 795 trimming rounds
		0.0000108345,
		
		// After 796 trimming rounds
		0.0000108199,
		
		// After 797 trimming rounds
		0.0000108052,
		
		// After 798 trimming rounds
		0.0000107905,
		
		// After 799 trimming rounds
		0.0000107763,
		
		// After 800 trimming rounds
		0.0000107621,
		
		// After 801 trimming rounds
		0.0000107484,
		
		// After 802 trimming rounds
		0.0000107349,
		
		// After 803 trimming rounds
		0.0000107214,
		
		// After 804 trimming rounds
		0.0000107086,
		
		// After 805 trimming rounds
		0.0000106958,
		
		// After 806 trimming rounds
		0.000010683,
		
		// After 807 trimming rounds
		0.0000106702,
		
		// After 808 trimming rounds
		0.0000106574,
		
		// After 809 trimming rounds
		0.0000106446,
		
		// After 810 trimming rounds
		0.000010632,
		
		// After 811 trimming rounds
		0.0000106192,
		
		// After 812 trimming rounds
		0.0000106064,
		
		// After 813 trimming rounds
		0.0000105936,
		
		// After 814 trimming rounds
		0.0000105808,
		
		// After 815 trimming rounds
		0.000010568,
		
		// After 816 trimming rounds
		0.0000105551,
		
		// After 817 trimming rounds
		0.0000105426,
		
		// After 818 trimming rounds
		0.00001053,
		
		// After 819 trimming rounds
		0.0000105174,
		
		// After 820 trimming rounds
		0.0000105049,
		
		// After 821 trimming rounds
		0.000010492,
		
		// After 822 trimming rounds
		0.000010479,
		
		// After 823 trimming rounds
		0.000010466,
		
		// After 824 trimming rounds
		0.0000104529,
		
		// After 825 trimming rounds
		0.0000104399,
		
		// After 826 trimming rounds
		0.0000104271,
		
		// After 827 trimming rounds
		0.0000104143,
		
		// After 828 trimming rounds
		0.0000104019,
		
		// After 829 trimming rounds
		0.0000103896,
		
		// After 830 trimming rounds
		0.0000103775,
		
		// After 831 trimming rounds
		0.0000103654,
		
		// After 832 trimming rounds
		0.0000103533,
		
		// After 833 trimming rounds
		0.0000103412,
		
		// After 834 trimming rounds
		0.0000103291,
		
		// After 835 trimming rounds
		0.000010317,
		
		// After 836 trimming rounds
		0.0000103049,
		
		// After 837 trimming rounds
		0.0000102927,
		
		// After 838 trimming rounds
		0.0000102806,
		
		// After 839 trimming rounds
		0.0000102685,
		
		// After 840 trimming rounds
		0.0000102564,
		
		// After 841 trimming rounds
		0.0000102441,
		
		// After 842 trimming rounds
		0.000010232,
		
		// After 843 trimming rounds
		0.0000102199,
		
		// After 844 trimming rounds
		0.0000102078,
		
		// After 845 trimming rounds
		0.0000101957,
		
		// After 846 trimming rounds
		0.0000101833,
		
		// After 847 trimming rounds
		0.000010171,
		
		// After 848 trimming rounds
		0.0000101584,
		
		// After 849 trimming rounds
		0.0000101458,
		
		// After 850 trimming rounds
		0.0000101333,
		
		// After 851 trimming rounds
		0.0000101207,
		
		// After 852 trimming rounds
		0.0000101081,
		
		// After 853 trimming rounds
		0.0000100955,
		
		// After 854 trimming rounds
		0.0000100832,
		
		// After 855 trimming rounds
		0.0000100709,
		
		// After 856 trimming rounds
		0.0000100585,
		
		// After 857 trimming rounds
		0.0000100462,
		
		// After 858 trimming rounds
		0.0000100338,
		
		// After 859 trimming rounds
		0.0000100215,
		
		// After 860 trimming rounds
		0.0000100094,
		
		// After 861 trimming rounds
		0.00000999752,
		
		// After 862 trimming rounds
		0.00000998564,
		
		// After 863 trimming rounds
		0.00000997377,
		
		// After 864 trimming rounds
		0.00000996189,
		
		// After 865 trimming rounds
		0.00000995002,
		
		// After 866 trimming rounds
		0.00000993814,
		
		// After 867 trimming rounds
		0.0000099265,
		
		// After 868 trimming rounds
		0.00000991486,
		
		// After 869 trimming rounds
		0.00000990322,
		
		// After 870 trimming rounds
		0.00000989158,
		
		// After 871 trimming rounds
		0.00000987994,
		
		// After 872 trimming rounds
		0.00000986853,
		
		// After 873 trimming rounds
		0.00000985712,
		
		// After 874 trimming rounds
		0.00000984571,
		
		// After 875 trimming rounds
		0.0000098343,
		
		// After 876 trimming rounds
		0.00000982266,
		
		// After 877 trimming rounds
		0.00000981102,
		
		// After 878 trimming rounds
		0.00000979914,
		
		// After 879 trimming rounds
		0.00000978727,
		
		// After 880 trimming rounds
		0.00000977539,
		
		// After 881 trimming rounds
		0.00000976352,
		
		// After 882 trimming rounds
		0.00000975211,
		
		// After 883 trimming rounds
		0.00000974094,
		
		// After 884 trimming rounds
		0.00000972976,
		
		// After 885 trimming rounds
		0.00000971858,
		
		// After 886 trimming rounds
		0.00000970741,
		
		// After 887 trimming rounds
		0.00000969623,
		
		// After 888 trimming rounds
		0.00000968506,
		
		// After 889 trimming rounds
		0.00000967411,
		
		// After 890 trimming rounds
		0.00000966317,
		
		// After 891 trimming rounds
		0.00000965246,
		
		// After 892 trimming rounds
		0.00000964175,
		
		// After 893 trimming rounds
		0.00000963104,
		
		// After 894 trimming rounds
		0.00000962033,
		
		// After 895 trimming rounds
		0.00000960962,
		
		// After 896 trimming rounds
		0.00000959914,
		
		// After 897 trimming rounds
		0.00000958866,
		
		// After 898 trimming rounds
		0.00000957819,
		
		// After 899 trimming rounds
		0.00000956794,
		
		// After 900 trimming rounds
		0.00000955793,
		
		// After 901 trimming rounds
		0.00000954792,
		
		// After 902 trimming rounds
		0.00000953791,
		
		// After 903 trimming rounds
		0.0000095279,
		
		// After 904 trimming rounds
		0.00000951788,
		
		// After 905 trimming rounds
		0.00000950787,
		
		// After 906 trimming rounds
		0.00000949763,
		
		// After 907 trimming rounds
		0.00000948738,
		
		// After 908 trimming rounds
		0.00000947714,
		
		// After 909 trimming rounds
		0.00000946689,
		
		// After 910 trimming rounds
		0.00000945688,
		
		// After 911 trimming rounds
		0.00000944687,
		
		// After 912 trimming rounds
		0.00000943686,
		
		// After 913 trimming rounds
		0.00000942685,
		
		// After 914 trimming rounds
		0.00000941684,
		
		// After 915 trimming rounds
		0.00000940682,
		
		// After 916 trimming rounds
		0.00000939681,
		
		// After 917 trimming rounds
		0.0000093868,
		
		// After 918 trimming rounds
		0.00000937679,
		
		// After 919 trimming rounds
		0.00000936678,
		
		// After 920 trimming rounds
		0.00000935677,
		
		// After 921 trimming rounds
		0.00000934675,
		
		// After 922 trimming rounds
		0.00000933674,
		
		// After 923 trimming rounds
		0.00000932673,
		
		// After 924 trimming rounds
		0.00000931695,
		
		// After 925 trimming rounds
		0.0000093074,
		
		// After 926 trimming rounds
		0.00000929786,
		
		// After 927 trimming rounds
		0.00000928831,
		
		// After 928 trimming rounds
		0.00000927877,
		
		// After 929 trimming rounds
		0.00000926945,
		
		// After 930 trimming rounds
		0.00000926014,
		
		// After 931 trimming rounds
		0.00000925083,
		
		// After 932 trimming rounds
		0.00000924151,
		
		// After 933 trimming rounds
		0.0000092322,
		
		// After 934 trimming rounds
		0.00000922289,
		
		// After 935 trimming rounds
		0.00000921381,
		
		// After 936 trimming rounds
		0.00000920473,
		
		// After 937 trimming rounds
		0.00000919565,
		
		// After 938 trimming rounds
		0.00000918657,
		
		// After 939 trimming rounds
		0.00000917749,
		
		// After 940 trimming rounds
		0.00000916817,
		
		// After 941 trimming rounds
		0.00000915886,
		
		// After 942 trimming rounds
		0.00000914955,
		
		// After 943 trimming rounds
		0.00000914023,
		
		// After 944 trimming rounds
		0.00000913115,
		
		// After 945 trimming rounds
		0.00000912207,
		
		// After 946 trimming rounds
		0.00000911299,
		
		// After 947 trimming rounds
		0.00000910391,
		
		// After 948 trimming rounds
		0.00000909483,
		
		// After 949 trimming rounds
		0.00000908575,
		
		// After 950 trimming rounds
		0.00000907667,
		
		// After 951 trimming rounds
		0.00000906759,
		
		// After 952 trimming rounds
		0.00000905851,
		
		// After 953 trimming rounds
		0.00000904943,
		
		// After 954 trimming rounds
		0.00000904035,
		
		// After 955 trimming rounds
		0.00000903127,
		
		// After 956 trimming rounds
		0.00000902219,
		
		// After 957 trimming rounds
		0.00000901334,
		
		// After 958 trimming rounds
		0.00000900449,
		
		// After 959 trimming rounds
		0.00000899564,
		
		// After 960 trimming rounds
		0.0000089868,
		
		// After 961 trimming rounds
		0.00000897795,
		
		// After 962 trimming rounds
		0.0000089691,
		
		// After 963 trimming rounds
		0.00000896025,
		
		// After 964 trimming rounds
		0.00000895141,
		
		// After 965 trimming rounds
		0.00000894256,
		
		// After 966 trimming rounds
		0.00000893371,
		
		// After 967 trimming rounds
		0.00000892486,
		
		// After 968 trimming rounds
		0.00000891602,
		
		// After 969 trimming rounds
		0.00000890717,
		
		// After 970 trimming rounds
		0.00000889832,
		
		// After 971 trimming rounds
		0.00000888971,
		
		// After 972 trimming rounds
		0.00000888109,
		
		// After 973 trimming rounds
		0.00000887248,
		
		// After 974 trimming rounds
		0.00000886386,
		
		// After 975 trimming rounds
		0.00000885525,
		
		// After 976 trimming rounds
		0.00000884663,
		
		// After 977 trimming rounds
		0.00000883802,
		
		// After 978 trimming rounds
		0.0000088294,
		
		// After 979 trimming rounds
		0.00000882079,
		
		// After 980 trimming rounds
		0.00000881217,
		
		// After 981 trimming rounds
		0.00000880356,
		
		// After 982 trimming rounds
		0.00000879494,
		
		// After 983 trimming rounds
		0.00000878633,
		
		// After 984 trimming rounds
		0.00000877772,
		
		// After 985 trimming rounds
		0.0000087691,
		
		// After 986 trimming rounds
		0.00000876049,
		
		// After 987 trimming rounds
		0.00000875187,
		
		// After 988 trimming rounds
		0.00000874326,
		
		// After 989 trimming rounds
		0.00000873464,
		
		// After 990 trimming rounds
		0.00000872603,
		
		// After 991 trimming rounds
		0.00000871741,
		
		// After 992 trimming rounds
		0.0000087088,
		
		// After 993 trimming rounds
		0.00000870018,
		
		// After 994 trimming rounds
		0.00000869157,
		
		// After 995 trimming rounds
		0.00000868295,
		
		// After 996 trimming rounds
		0.00000867434,
		
		// After 997 trimming rounds
		0.00000866572,
		
		// After 998 trimming rounds
		0.00000865711,
		
		// After 999 trimming rounds
		0.00000864849,
		
		// After 1000 trimming rounds
		0.00000863988
	};
	
	// Throw error if number of trimming rounds is invalid
	assert(("Number of trimming rounds is invalid", numberOfTrimmingRounds < sizeof(percentOfEdgesRemainingAfterNumberOfTrimmingRounds) / sizeof(percentOfEdgesRemainingAfterNumberOfTrimmingRounds[0])));
	
	// Get the number of edges remaining
	const double numberOfEdgesRemaining = NUMBER_OF_EDGES * percentOfEdgesRemainingAfterNumberOfTrimmingRounds[numberOfTrimmingRounds];
	
	// Check if additional space tolerance is based on the initial number of edges
	double additionalSpaceTolerance;
	if(additionalSpaceToleranceBasedOnInitialNumberOfEdges) [[likely]] {
	
		// Set additional space tolerance based on the initial number of edges
		additionalSpaceTolerance = additionalSpaceTolerancePercent(EDGE_BITS);
	}
	
	// Otherwise
	else [[unlikely]] {
	
		// Set additional space tolerance based on the current number of edges
		additionalSpaceTolerance = additionalSpaceTolerancePercent(bit_width(max(ceilAsUint64(numberOfEdgesRemaining), static_cast<uint64_t>(1)) - 1));
	}
	
	// Return the number of edges remaining with some additional space tolerance
	return ceilAsUint64(numberOfEdgesRemaining * additionalSpaceTolerance);
}

// Set thread priority and affinity
__attribute__((always_inline)) static inline bool setThreadPriorityAndAffinity(unsigned int cpuCoreIndex) noexcept {

	// Check if using Windows
	#ifdef _WIN32
	
		// Check if setting thread's scheduling priority to max failed
		if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if getting system CPU set information size failed
		ULONG systemCpuSetInformationSize;
		if(GetSystemCpuSetInformation(nullptr, 0, &systemCpuSetInformationSize, nullptr, 0) || GetLastError() != ERROR_INSUFFICIENT_BUFFER || !systemCpuSetInformationSize) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if getting system CPU set information failed
		alignas(SYSTEM_CPU_SET_INFORMATION) uint8_t systemCpuSetInformation[systemCpuSetInformationSize];
		if(!GetSystemCpuSetInformation(reinterpret_cast<SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation), systemCpuSetInformationSize, &systemCpuSetInformationSize, nullptr, 0)) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Go through all system CPU sets
		BYTE highestEfficiencyClass = 0;
		ULONG initialSystemCpuSetInformationSize = systemCpuSetInformationSize;
		for(size_t i = 0; systemCpuSetInformationSize > 0; systemCpuSetInformationSize -= reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i].Size, ++i) [[likely]] {
		
			// Get CPU set information
			const SYSTEM_CPU_SET_INFORMATION &cpuSetInformation = reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i];
			
			// Check if CPU set information is for a CPU core that is available
			if(cpuSetInformation.Type == CpuSetInformation && !cpuSetInformation.CpuSet.Allocated && !cpuSetInformation.CpuSet.Parked) [[likely]] {
			
				// Update highest efficiency class
				highestEfficiencyClass = max(highestEfficiencyClass, cpuSetInformation.CpuSet.EfficiencyClass);
			}
		}
		
		// Go through all system CPU sets
		for(size_t i = 0; initialSystemCpuSetInformationSize > 0; initialSystemCpuSetInformationSize -= reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i].Size, ++i) [[likely]] {
		
			// Get CPU set information
			const SYSTEM_CPU_SET_INFORMATION &cpuSetInformation = reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i];
			
			// Check if CPU set information is for a CPU core that is available and is high performance
			if(cpuSetInformation.Type == CpuSetInformation && !cpuSetInformation.CpuSet.Allocated && !cpuSetInformation.CpuSet.Parked && cpuSetInformation.CpuSet.EfficiencyClass == highestEfficiencyClass) [[likely]] {
			
				// Check if this CPU core is at the specified CPU core index
				if(!cpuCoreIndex--) [[unlikely]] {
				
					// Check if setting thread's CPU affinity to its own core was successful
					const GROUP_AFFINITY groupAffinity = {
					
						// Mask
						.Mask = static_cast<KAFFINITY>(1) << cpuSetInformation.CpuSet.LogicalProcessorIndex,
						
						// Group
						.Group = cpuSetInformation.CpuSet.Group
					};
					
					if(SetThreadGroupAffinity(GetCurrentThread(), &groupAffinity, nullptr)) [[likely]] {
					
						// Return true
						return true;
					}
					
					// Break
					break;
				}
			}
		}
		
	// Otherwise check if using an Apple device
	#elif defined __APPLE__
	
		// Check if setting thread's scheduling priority to max failed
		if(pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if setting thread's CPU affinity to its own core failed
		alignas(remove_pointer_t<thread_policy_t>) thread_affinity_policy_data_t affinityPolicy = {
		
			// Affinity tag
			.affinity_tag = static_cast<integer_t>(cpuCoreIndex + 1)
		};
		
		const kern_return_t result = thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&affinityPolicy), THREAD_AFFINITY_POLICY_COUNT);
		if(result != KERN_SUCCESS && result != KERN_NOT_SUPPORTED) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Return true
		return true;
		
	// Otherwise
	#else
	
		// Check if getting thread's scheduling policy failed
		int schedulingPolicy;
		sched_param schedulingParameters;
		if(pthread_getschedparam(pthread_self(), &schedulingPolicy, &schedulingParameters)) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if getting the max priority for the thread's scheduling policy failed
		schedulingParameters.sched_priority = sched_get_priority_max(schedulingPolicy);
		if(schedulingParameters.sched_priority == -1) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if setting thread's scheduling priority to max failed
		if(pthread_setschedparam(pthread_self(), schedulingPolicy, &schedulingParameters)) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if opening CPU directory failed
		const unique_ptr<DIR, decltype(&closedir)> cpuDirectory(opendir("/sys/devices/system/cpu"), closedir);
		if(!cpuDirectory) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Go through all entries in the CPU directory
		unsigned long long highestCpuFrequency = 0;
		dirent *entry;
		while((entry = readdir(cpuDirectory.get()))) [[likely]] {
		
			// Check if entry is a CPU core
			if(entry->d_type == DT_DIR && !__builtin_strncmp(entry->d_name, "cpu", sizeof("cpu") - sizeof('\0'))) [[unlikely]] {
			
				// Check if getting CPU's core number was successful
				const char *cpuCore = &entry->d_name[sizeof("cpu") - sizeof('\0')];
				char *end;
				errno = 0;
				const unsigned long long cpuCoreAsNumber = strtoull(cpuCore, &end, DECIMAL_NUMBER_BASE);
				if(end != cpuCore && !*end && isdigit(cpuCore[0]) && (cpuCore[0] != '0' || !isdigit(cpuCore[sizeof('0')])) && !errno && cpuCoreAsNumber <= UINT64_MAX) [[likely]] {
				
					// Create CPU file path
					char cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof("/cpufreq/cpuinfo_max_freq")] = "/sys/devices/system/cpu/cpu";
					
					// Append CPU core number to CPU file path
					const to_chars_result appendResult = to_chars(&cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')], &cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')] + MAX_UINT64_STRING_SIZE, cpuCoreAsNumber);
					
					// Append end of CPU online file path to CPU file path
					__builtin_memcpy(appendResult.ptr, "/online", sizeof("/online"));
					
					// Check if opening CPU online file was successful or exists but failed to open
					const unique_ptr<FILE, decltype(&fclose)> cpuOnlineFile(fopen(cpuFilePath, "rb"), fclose);
					if(cpuOnlineFile || errno != ENOENT) [[likely]] {
					
						// Check if CPU online file exists but failed to open or CPU isn't available
						if((!cpuOnlineFile && errno != ENOENT) || fgetc(cpuOnlineFile.get()) != '1') [[unlikely]] {
						
							// Continue
							continue;
						}
					}
					
					// Append end of CPU frequency file path to CPU file path
					__builtin_memcpy(appendResult.ptr, "/cpufreq/cpuinfo_max_freq", sizeof("/cpufreq/cpuinfo_max_freq"));
					
					// Check if opening CPU frequency file was successful
					const unique_ptr<FILE, decltype(&fclose)> cpuFrequencyFile(fopen(cpuFilePath, "rb"), fclose);
					if(cpuFrequencyFile) [[likely]] {
					
						// Check if reading the CPU frequency file was successful
						char cpuFrequency[MAX_UINT64_STRING_SIZE + sizeof("\n")];
						const size_t cpuFrequencySize = fread(cpuFrequency, sizeof(char), sizeof(cpuFrequency), cpuFrequencyFile.get());
						
						if(cpuFrequencySize && cpuFrequencySize <= MAX_UINT64_STRING_SIZE + sizeof('\n') && !__builtin_memchr(cpuFrequency, '\0', cpuFrequencySize)) [[likely]] {
						
							// Make CPU frequency a string
							cpuFrequency[cpuFrequencySize - (__builtin_expect(cpuFrequency[cpuFrequencySize - sizeof('\n')] == '\n', true) ? sizeof('\n') : 0)] = '\0';
							
							// Check if getting CPU's frequency number was successful
							errno = 0;
							const unsigned long long cpuFrequencyAsNumber = strtoull(cpuFrequency, &end, DECIMAL_NUMBER_BASE);
							if(end != cpuFrequency && !*end && isdigit(cpuFrequency[0]) && (cpuFrequency[0] != '0' || !isdigit(cpuFrequency[sizeof('0')])) && !errno) [[likely]] {
							
								// Update highest CPU frequency
								highestCpuFrequency = max(highestCpuFrequency, cpuFrequencyAsNumber);
							}
						}
					}
				}
			}
		}
		
		// Go through all entries in the CPU directory
		rewinddir(cpuDirectory.get());
		while((entry = readdir(cpuDirectory.get()))) [[likely]] {
		
			// Check if entry is a CPU core
			if(entry->d_type == DT_DIR && !__builtin_strncmp(entry->d_name, "cpu", sizeof("cpu") - sizeof('\0'))) [[unlikely]] {
			
				// Check if getting CPU's core number was successful
				const char *cpuCore = &entry->d_name[sizeof("cpu") - sizeof('\0')];
				char *end;
				errno = 0;
				const unsigned long long cpuCoreAsNumber = strtoull(cpuCore, &end, DECIMAL_NUMBER_BASE);
				if(end != cpuCore && !*end && isdigit(cpuCore[0]) && (cpuCore[0] != '0' || !isdigit(cpuCore[sizeof('0')])) && !errno && cpuCoreAsNumber <= UINT64_MAX) [[likely]] {
				
					// Create CPU file path
					char cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof("/cpufreq/cpuinfo_max_freq")] = "/sys/devices/system/cpu/cpu";
					
					// Append CPU core number to CPU file path
					const to_chars_result appendResult = to_chars(&cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')], &cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')] + MAX_UINT64_STRING_SIZE, cpuCoreAsNumber);
					
					// Append end of CPU online file path to CPU file path
					__builtin_memcpy(appendResult.ptr, "/online", sizeof("/online"));
					
					// Check if opening CPU online file was successful or exists but failed to open
					const unique_ptr<FILE, decltype(&fclose)> cpuOnlineFile(fopen(cpuFilePath, "rb"), fclose);
					if(cpuOnlineFile || errno != ENOENT) [[likely]] {
					
						// Check if CPU online file exists but failed to open or CPU isn't available
						if((!cpuOnlineFile && errno != ENOENT) || fgetc(cpuOnlineFile.get()) != '1') [[unlikely]] {
						
							// Continue
							continue;
						}
					}
					
					// Append end of CPU frequency file path to CPU file path
					__builtin_memcpy(appendResult.ptr, "/cpufreq/cpuinfo_max_freq", sizeof("/cpufreq/cpuinfo_max_freq"));
					
					// Check if opening CPU frequency file was successful
					const unique_ptr<FILE, decltype(&fclose)> cpuFrequencyFile(fopen(cpuFilePath, "rb"), fclose);
					if(cpuFrequencyFile) [[likely]] {
					
						// Check if reading the CPU frequency file was successful
						char cpuFrequency[MAX_UINT64_STRING_SIZE + sizeof("\n")];
						const size_t cpuFrequencySize = fread(cpuFrequency, sizeof(char), sizeof(cpuFrequency), cpuFrequencyFile.get());
						
						if(cpuFrequencySize && cpuFrequencySize <= MAX_UINT64_STRING_SIZE + sizeof('\n') && !__builtin_memchr(cpuFrequency, '\0', cpuFrequencySize)) [[likely]] {
						
							// Make CPU frequency a string
							cpuFrequency[cpuFrequencySize - (__builtin_expect(cpuFrequency[cpuFrequencySize - sizeof('\n')] == '\n', true) ? sizeof('\n') : 0)] = '\0';
							
							// Check if getting CPU's frequency number was successful and CPU core is high performance
							errno = 0;
							const unsigned long long cpuFrequencyAsNumber = strtoull(cpuFrequency, &end, DECIMAL_NUMBER_BASE);
							if(end != cpuFrequency && !*end && isdigit(cpuFrequency[0]) && (cpuFrequency[0] != '0' || !isdigit(cpuFrequency[sizeof('0')])) && !errno && cpuFrequencyAsNumber == highestCpuFrequency) [[likely]] {
							
								// Check if this CPU core is at the specified CPU core index
								if(!cpuCoreIndex--) [[unlikely]] {
								
									// Check if setting thread's CPU affinity to its own core was successful
									cpu_set_t cpuSet;
									CPU_ZERO(&cpuSet);
									CPU_SET(cpuCoreAsNumber, &cpuSet);
									if(!pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet)) [[likely]] {
									
										// Return true
										return true;
									}
									
									// Break
									break;
								}
							}
						}
					}
				}
			}
		}
	#endif
	
	// Return false
	return false;
}

// Get number of high performance CPU cores
__attribute__((always_inline)) static inline unsigned int getNumberOfHighPerformanceCpuCores() noexcept {

	// Check if using Windows
	#ifdef _WIN32
	
		// Check if getting system CPU set information size was successful
		ULONG systemCpuSetInformationSize;
		if(!GetSystemCpuSetInformation(nullptr, 0, &systemCpuSetInformationSize, nullptr, 0) && GetLastError() == ERROR_INSUFFICIENT_BUFFER && systemCpuSetInformationSize) [[likely]] {
		
			// Check if getting system CPU set information was successful
			alignas(SYSTEM_CPU_SET_INFORMATION) uint8_t systemCpuSetInformation[systemCpuSetInformationSize];
			if(GetSystemCpuSetInformation(reinterpret_cast<SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation), systemCpuSetInformationSize, &systemCpuSetInformationSize, nullptr, 0)) [[likely]] {
			
				// Go through all system CPU sets
				BYTE highestEfficiencyClass = 0;
				ULONG initialSystemCpuSetInformationSize = systemCpuSetInformationSize;
				for(size_t i = 0; systemCpuSetInformationSize > 0; systemCpuSetInformationSize -= reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i].Size, ++i) [[likely]] {
				
					// Get CPU set information
					const SYSTEM_CPU_SET_INFORMATION &cpuSetInformation = reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i];
					
					// Check if CPU set information is for a CPU core that is available
					if(cpuSetInformation.Type == CpuSetInformation && !cpuSetInformation.CpuSet.Allocated && !cpuSetInformation.CpuSet.Parked) [[likely]] {
					
						// Update highest efficiency class
						highestEfficiencyClass = max(highestEfficiencyClass, cpuSetInformation.CpuSet.EfficiencyClass);
					}
				}
				
				// Go through all system CPU sets
				unsigned int numberOfHighPerformanceCpuCores = 0;
				for(size_t i = 0; initialSystemCpuSetInformationSize > 0; initialSystemCpuSetInformationSize -= reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i].Size, ++i) [[likely]] {
				
					// Get CPU set information
					const SYSTEM_CPU_SET_INFORMATION &cpuSetInformation = reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION *>(systemCpuSetInformation)[i];
					
					// Check if CPU set information is for a CPU core that is available and is high performance
					if(cpuSetInformation.Type == CpuSetInformation && !cpuSetInformation.CpuSet.Allocated && !cpuSetInformation.CpuSet.Parked && cpuSetInformation.CpuSet.EfficiencyClass == highestEfficiencyClass) [[likely]] {
					
						// Check if incrementing number of high performance CPU cores is at its max value
						if(++numberOfHighPerformanceCpuCores == UINT_MAX) [[unlikely]] {
						
							// Break
							break;
						}
					}
				}
				
				// Return number of high performance CPU cores
				return max(numberOfHighPerformanceCpuCores, static_cast<unsigned int>(1));
			}
		}
	
	// Otherwise check if using an Apple device
	#elif defined __APPLE__
	
		// Check if getting the number of high performance CPU cores was successful
		unsigned long long numberOfHighPerformanceCpuCores = 0;
		size_t numberOfHighPerformanceCpuCoresSize = sizeof(numberOfHighPerformanceCpuCores);
		if(!sysctlbyname("hw.perflevel0.logicalcpu", &numberOfHighPerformanceCpuCores, &numberOfHighPerformanceCpuCoresSize, nullptr, 0)) [[likely]] {
		
			// Return number of high performance CPU cores
			return max(__builtin_expect(numberOfHighPerformanceCpuCores > UINT_MAX, false) ? UINT_MAX : static_cast<unsigned int>(numberOfHighPerformanceCpuCores), static_cast<unsigned int>(1));
		}
		
	// Otherwise
	#else
	
		// Check if opening CPU directory was successful
		const unique_ptr<DIR, decltype(&closedir)> cpuDirectory(opendir("/sys/devices/system/cpu"), closedir);
		if(cpuDirectory) [[likely]] {
		
			// Go through all entries in the CPU directory
			unsigned long long highestCpuFrequency = 0;
			dirent *entry;
			while((entry = readdir(cpuDirectory.get()))) [[likely]] {
			
				// Check if entry is a CPU core
				if(entry->d_type == DT_DIR && !__builtin_strncmp(entry->d_name, "cpu", sizeof("cpu") - sizeof('\0'))) [[unlikely]] {
				
					// Check if getting CPU's core number was successful
					const char *cpuCore = &entry->d_name[sizeof("cpu") - sizeof('\0')];
					char *end;
					errno = 0;
					const unsigned long long cpuCoreAsNumber = strtoull(cpuCore, &end, DECIMAL_NUMBER_BASE);
					if(end != cpuCore && !*end && isdigit(cpuCore[0]) && (cpuCore[0] != '0' || !isdigit(cpuCore[sizeof('0')])) && !errno && cpuCoreAsNumber <= UINT64_MAX) [[likely]] {
					
						// Create CPU file path
						char cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof("/cpufreq/cpuinfo_max_freq")] = "/sys/devices/system/cpu/cpu";
						
						// Append CPU core number to CPU file path
						const to_chars_result appendResult = to_chars(&cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')], &cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')] + MAX_UINT64_STRING_SIZE, cpuCoreAsNumber);
						
						// Append end of CPU online file path to CPU file path
						__builtin_memcpy(appendResult.ptr, "/online", sizeof("/online"));
						
						// Check if opening CPU online file was successful or exists but failed to open
						const unique_ptr<FILE, decltype(&fclose)> cpuOnlineFile(fopen(cpuFilePath, "rb"), fclose);
						if(cpuOnlineFile || errno != ENOENT) [[likely]] {
						
							// Check if CPU online file exists but failed to open or CPU isn't available
							if((!cpuOnlineFile && errno != ENOENT) || fgetc(cpuOnlineFile.get()) != '1') [[unlikely]] {
							
								// Continue
								continue;
							}
						}
						
						// Append end of CPU frequency file path to CPU file path
						__builtin_memcpy(appendResult.ptr, "/cpufreq/cpuinfo_max_freq", sizeof("/cpufreq/cpuinfo_max_freq"));
						
						// Check if opening CPU frequency file was successful
						const unique_ptr<FILE, decltype(&fclose)> cpuFrequencyFile(fopen(cpuFilePath, "rb"), fclose);
						if(cpuFrequencyFile) [[likely]] {
						
							// Check if reading the CPU frequency file was successful
							char cpuFrequency[MAX_UINT64_STRING_SIZE + sizeof("\n")];
							const size_t cpuFrequencySize = fread(cpuFrequency, sizeof(char), sizeof(cpuFrequency), cpuFrequencyFile.get());
							
							if(cpuFrequencySize && cpuFrequencySize <= MAX_UINT64_STRING_SIZE + sizeof('\n') && !__builtin_memchr(cpuFrequency, '\0', cpuFrequencySize)) [[likely]] {
							
								// Make CPU frequency a string
								cpuFrequency[cpuFrequencySize - (__builtin_expect(cpuFrequency[cpuFrequencySize - sizeof('\n')] == '\n', true) ? sizeof('\n') : 0)] = '\0';
								
								// Check if getting CPU's frequency number was successful
								errno = 0;
								const unsigned long long cpuFrequencyAsNumber = strtoull(cpuFrequency, &end, DECIMAL_NUMBER_BASE);
								if(end != cpuFrequency && !*end && isdigit(cpuFrequency[0]) && (cpuFrequency[0] != '0' || !isdigit(cpuFrequency[sizeof('0')])) && !errno) [[likely]] {
								
									// Update highest CPU frequency
									highestCpuFrequency = max(highestCpuFrequency, cpuFrequencyAsNumber);
								}
							}
						}
					}
				}
			}
			
			// Go through all entries in the CPU directory
			rewinddir(cpuDirectory.get());
			unsigned int numberOfHighPerformanceCpuCores = 0;
			while((entry = readdir(cpuDirectory.get()))) [[likely]] {
			
				// Check if entry is a CPU core
				if(entry->d_type == DT_DIR && !__builtin_strncmp(entry->d_name, "cpu", sizeof("cpu") - sizeof('\0'))) [[unlikely]] {
				
					// Check if getting CPU's core number was successful
					const char *cpuCore = &entry->d_name[sizeof("cpu") - sizeof('\0')];
					char *end;
					errno = 0;
					const unsigned long long cpuCoreAsNumber = strtoull(cpuCore, &end, DECIMAL_NUMBER_BASE);
					if(end != cpuCore && !*end && isdigit(cpuCore[0]) && (cpuCore[0] != '0' || !isdigit(cpuCore[sizeof('0')])) && !errno && cpuCoreAsNumber <= UINT64_MAX) [[likely]] {
					
						// Create CPU file path
						char cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof("/cpufreq/cpuinfo_max_freq")] = "/sys/devices/system/cpu/cpu";
						
						// Append CPU core number to CPU file path
						const to_chars_result appendResult = to_chars(&cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')], &cpuFilePath[sizeof("/sys/devices/system/cpu/cpu") - sizeof('\0')] + MAX_UINT64_STRING_SIZE, cpuCoreAsNumber);
						
						// Append end of CPU online file path to CPU file path
						__builtin_memcpy(appendResult.ptr, "/online", sizeof("/online"));
						
						// Check if opening CPU online file was successful or exists but failed to open
						const unique_ptr<FILE, decltype(&fclose)> cpuOnlineFile(fopen(cpuFilePath, "rb"), fclose);
						if(cpuOnlineFile || errno != ENOENT) [[likely]] {
						
							// Check if CPU online file exists but failed to open or CPU isn't available
							if((!cpuOnlineFile && errno != ENOENT) || fgetc(cpuOnlineFile.get()) != '1') [[unlikely]] {
							
								// Continue
								continue;
							}
						}
						
						// Append end of CPU frequency file path to CPU file path
						__builtin_memcpy(appendResult.ptr, "/cpufreq/cpuinfo_max_freq", sizeof("/cpufreq/cpuinfo_max_freq"));
						
						// Check if opening CPU frequency file was successful
						const unique_ptr<FILE, decltype(&fclose)> cpuFrequencyFile(fopen(cpuFilePath, "rb"), fclose);
						if(cpuFrequencyFile) [[likely]] {
						
							// Check if reading the CPU frequency file was successful
							char cpuFrequency[MAX_UINT64_STRING_SIZE + sizeof("\n")];
							const size_t cpuFrequencySize = fread(cpuFrequency, sizeof(char), sizeof(cpuFrequency), cpuFrequencyFile.get());
							
							if(cpuFrequencySize && cpuFrequencySize <= MAX_UINT64_STRING_SIZE + sizeof('\n') && !__builtin_memchr(cpuFrequency, '\0', cpuFrequencySize)) [[likely]] {
							
								// Make CPU frequency a string
								cpuFrequency[cpuFrequencySize - (__builtin_expect(cpuFrequency[cpuFrequencySize - sizeof('\n')] == '\n', true) ? sizeof('\n') : 0)] = '\0';
								
								// Check if getting CPU's frequency number was successful and CPU core is high performance
								errno = 0;
								const unsigned long long cpuFrequencyAsNumber = strtoull(cpuFrequency, &end, DECIMAL_NUMBER_BASE);
								if(end != cpuFrequency && !*end && isdigit(cpuFrequency[0]) && (cpuFrequency[0] != '0' || !isdigit(cpuFrequency[sizeof('0')])) && !errno && cpuFrequencyAsNumber == highestCpuFrequency) [[likely]] {
								
									// Check if incrementing number of high performance CPU cores is at its max value
									if(++numberOfHighPerformanceCpuCores == UINT_MAX) [[unlikely]] {
									
										// Break
										break;
									}
								}
							}
						}
					}
				}
			}
			
			// Return number of high performance CPU cores
			return max(numberOfHighPerformanceCpuCores, static_cast<unsigned int>(1));
		}
	#endif
	
	// Return number of CPU cores
	return max(thread::hardware_concurrency(), static_cast<unsigned int>(1));
}


#endif
