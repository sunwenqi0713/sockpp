/**
 * @file Ftp.h
 * @brief FTP client implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>
#include <sockpp/IpAddress.h>
#include <sockpp/TcpSocket.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace sockpp {

/**
 * @brief A FTP client.
 */
class SOCKPP_API Ftp {
 public:
  /**
   * @brief Enumeration of transfer modes.
   */
  enum class TransferMode {
    Binary,  ///< Binary mode (file is transferred as a sequence of bytes).
    Ascii,   ///< Text mode using ASCII encoding.
    Ebcdic   ///< Text mode using EBCDIC encoding.
  };

  /**
   * @brief Define a FTP response.
   */
  class SOCKPP_API Response {
   public:
    /**
     * @brief Status codes possibly returned by a FTP response.
     */
    enum class Status {
      // 1xx: the requested action is being initiated.
      RestartMarkerReply = 110,           ///< Restart marker reply.
      ServiceReadySoon = 120,             ///< Service ready in N minutes.
      DataConnectionAlreadyOpened = 125,  ///< Data connection already opened.
      OpeningDataConnection = 150,        ///< File status ok, about to open data connection.

      // 2xx: the requested action has been successfully completed.
      Ok = 200,                     ///< Command ok.
      PointlessCommand = 202,       ///< Command not implemented.
      SystemStatus = 211,           ///< System status, or system help reply.
      DirectoryStatus = 212,        ///< Directory status.
      FileStatus = 213,             ///< File status.
      HelpMessage = 214,            ///< Help message.
      SystemType = 215,             ///< NAME system type.
      ServiceReady = 220,           ///< Service ready for new user.
      ClosingConnection = 221,      ///< Service closing control connection.
      DataConnectionOpened = 225,   ///< Data connection open, no transfer in progress.
      ClosingDataConnection = 226,  ///< Closing data connection.
      EnteringPassiveMode = 227,    ///< Entering passive mode.
      LoggedIn = 230,               ///< User logged in, proceed.
      FileActionOk = 250,           ///< Requested file action ok.
      DirectoryOk = 257,            ///< PATHNAME created.

      // 3xx: the command has been accepted.
      NeedPassword = 331,        ///< User name ok, need password.
      NeedAccountToLogIn = 332,  ///< Need account for login.
      NeedInformation = 350,     ///< Requested file action pending further information.

      // 4xx: the command was not accepted.
      ServiceUnavailable = 421,         ///< Service not available.
      DataConnectionUnavailable = 425,  ///< Can't open data connection.
      TransferAborted = 426,            ///< Connection closed, transfer aborted.
      FileActionAborted = 450,          ///< Requested file action not taken.
      LocalError = 451,                 ///< Requested action aborted.
      InsufficientStorageSpace = 452,   ///< Requested action not taken.

      // 5xx: the command was not accepted.
      CommandUnknown = 500,           ///< Syntax error, command unrecognized.
      ParametersUnknown = 501,        ///< Syntax error in parameters.
      CommandNotImplemented = 502,    ///< Command not implemented.
      BadCommandSequence = 503,       ///< Bad sequence of commands.
      ParameterNotImplemented = 504,  ///< Command not implemented for that parameter.
      NotLoggedIn = 530,              ///< Not logged in.
      NeedAccountToStore = 532,       ///< Need account for storing files.
      FileUnavailable = 550,          ///< Requested action not taken.
      PageTypeUnknown = 551,          ///< Requested action aborted.
      NotEnoughMemory = 552,          ///< Requested file action aborted.
      FilenameNotAllowed = 553,       ///< Requested action not taken.

      // 10xx: custom codes.
      InvalidResponse = 1000,   ///< Not a valid FTP response.
      ConnectionFailed = 1001,  ///< Connection with server failed.
      ConnectionClosed = 1002,  ///< Connection with server closed.
      InvalidFile = 1003        ///< Invalid file to upload / download.
    };

    /**
     * @brief Default constructor.
     *
     * This constructor is used by the FTP client to build
     * the response.
     *
     * @param code Response status code.
     * @param message Response message.
     */
    explicit Response(Status code = Status::InvalidResponse, std::string message = "");

    /**
     * @brief Check if the status code means a success.
     *
     * This function is defined for convenience, it is
     * equivalent to testing if the status code is < 400.
     *
     * @return true if the status is a success, false if it is a failure.
     */
    [[nodiscard]] bool isOk() const;

    /**
     * @brief Get the status code of the response.
     * @return Status code.
     */
    [[nodiscard]] Status getStatus() const;

    /**
     * @brief Get the full message contained in the response.
     * @return The response message.
     */
    [[nodiscard]] const std::string& getMessage() const;

   private:
    Status m_status;        ///< Status code returned from the server.
    std::string m_message;  ///< Last message received from the server.
  };

  /**
   * @brief Specialization of FTP response returning a directory.
   */
  class SOCKPP_API DirectoryResponse : public Response {
   public:
    /**
     * @brief Default constructor.
     * @param response Source response.
     */
    DirectoryResponse(const Response& response);

    /**
     * @brief Get the directory returned in the response.
     * @return Directory name.
     */
    [[nodiscard]] const std::filesystem::path& getDirectory() const;

   private:
    std::filesystem::path m_directory;  ///< Directory extracted from the response message.
  };

  /**
   * @brief Specialization of FTP response returning a filename listing.
   */
  class SOCKPP_API ListingResponse : public Response {
   public:
    /**
     * @brief Default constructor.
     * @param response Source response.
     * @param data Data containing the raw listing.
     */
    ListingResponse(const Response& response, std::string data);

    /**
     * @brief Return the array of directory/file names.
     * @return Array containing the requested listing.
     */
    [[nodiscard]] const std::vector<std::filesystem::path>& getListing() const;

   private:
    std::vector<std::filesystem::path> m_listing;  ///< Directory/file names extracted from the data.
  };

  /**
   * @brief Destructor.
   *
   * Automatically closes the connection with the server if
   * it is still opened.
   */
  ~Ftp();

  /**
   * @brief Connect to the specified FTP server.
   *
   * The port has a default value of 21, which is the standard
   * port used by the FTP protocol. You shouldn't use a different
   * value, unless you really know what you do.
   * This function tries to connect to the server so it may take
   * a while to complete, especially if the server is not
   * reachable. To avoid blocking your application for too long,
   * you can use a timeout.
   *
   * @param server Name or address of the FTP server to connect to.
   * @param port Port used for the connection.
   * @param timeout Maximum time to wait.
   * @return Server response to the request.
   * @see disconnect
   */
  [[nodiscard]] Response connect(IpAddress server, unsigned short port = 21,
                                 std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

  /**
   * @brief Close the connection with the server.
   * @return Server response to the request.
   * @see connect
   */
  [[nodiscard]] Response disconnect();

  /**
   * @brief Log in using an anonymous account.
   *
   * Logging in is mandatory after connecting to the server.
   * Users that are not logged in cannot perform any operation.
   *
   * @return Server response to the request.
   */
  [[nodiscard]] Response login();

  /**
   * @brief Log in using a username and a password.
   *
   * Logging in is mandatory after connecting to the server.
   * Users that are not logged in cannot perform any operation.
   *
   * @param name User name.
   * @param password Password.
   * @return Server response to the request.
   */
  [[nodiscard]] Response login(const std::string& name, const std::string& password);

  /**
   * @brief Send a null command to keep the connection alive.
   *
   * This command is useful because the server may close the
   * connection automatically if no command is sent.
   *
   * @return Server response to the request.
   */
  [[nodiscard]] Response keepAlive();

  /**
   * @brief Get the current working directory.
   *
   * The working directory is the root path for subsequent
   * operations involving directories and/or filenames.
   *
   * @return Server response to the request.
   * @see getDirectoryListing, changeDirectory, parentDirectory
   */
  [[nodiscard]] DirectoryResponse getWorkingDirectory();

  /**
   * @brief Get the contents of the given directory.
   *
   * This function retrieves the sub-directories and files
   * contained in the given directory. It is not recursive.
   * The @a directory parameter is relative to the current
   * working directory.
   *
   * @param directory Directory to list.
   * @return Server response to the request.
   * @see getWorkingDirectory, changeDirectory, parentDirectory
   */
  [[nodiscard]] ListingResponse getDirectoryListing(const std::string& directory = "");

  /**
   * @brief Change the current working directory.
   *
   * The new directory must be relative to the current one.
   *
   * @param directory New working directory.
   * @return Server response to the request.
   * @see getWorkingDirectory, getDirectoryListing, parentDirectory
   */
  [[nodiscard]] Response changeDirectory(const std::string& directory);

  /**
   * @brief Go to the parent directory of the current one.
   * @return Server response to the request.
   * @see getWorkingDirectory, getDirectoryListing, changeDirectory
   */
  [[nodiscard]] Response parentDirectory();

  /**
   * @brief Create a new directory.
   *
   * The new directory is created as a child of the current
   * working directory.
   *
   * @param name Name of the directory to create.
   * @return Server response to the request.
   * @see deleteDirectory
   */
  [[nodiscard]] Response createDirectory(const std::string& name);

  /**
   * @brief Remove an existing directory.
   *
   * The directory to remove must be relative to the
   * current working directory.
   * Use this function with caution, the directory will
   * be removed permanently!
   *
   * @param name Name of the directory to remove.
   * @return Server response to the request.
   * @see createDirectory
   */
  [[nodiscard]] Response deleteDirectory(const std::string& name);

  /**
   * @brief Rename an existing file.
   *
   * The filenames must be relative to the current working
   * directory.
   *
   * @param file File to rename.
   * @param newName New name of the file.
   * @return Server response to the request.
   * @see deleteFile
   */
  [[nodiscard]] Response renameFile(const std::string& file, const std::string& newName);

  /**
   * @brief Remove an existing file.
   *
   * The file name must be relative to the current working
   * directory.
   * Use this function with caution, the file will be
   * removed permanently!
   *
   * @param name File to remove.
   * @return Server response to the request.
   * @see renameFile
   */
  [[nodiscard]] Response deleteFile(const std::string& name);

  /**
   * @brief Download a file from the server.
   *
   * The filename of the distant file is relative to the
   * current working directory of the server, and the local
   * destination path is relative to the current directory
   * of your application.
   * If a file with the same filename as the distant file
   * already exists in the local destination path, it will
   * be overwritten.
   *
   * @param remoteFile Filename of the distant file to download.
   * @param localPath The directory in which to put the file on the local computer.
   * @param mode Transfer mode.
   * @return Server response to the request.
   * @see upload
   */
  [[nodiscard]] Response download(const std::string& remoteFile, const std::string& localPath,
                                  TransferMode mode = TransferMode::Binary);

  /**
   * @brief Upload a file to the server.
   *
   * The name of the local file is relative to the current
   * working directory of your application, and the
   * remote path is relative to the current directory of the
   * FTP server.
   *
   * The append parameter controls whether the remote file is
   * appended to or overwritten if it already exists.
   *
   * @param localFile Path of the local file to upload.
   * @param remotePath The directory in which to put the file on the server.
   * @param mode Transfer mode.
   * @param append Pass true to append to an existing file; false to overwrite it.
   * @return Server response to the request.
   * @see download
   */
  [[nodiscard]] Response upload(const std::string& localFile, const std::string& remotePath,
                                TransferMode mode = TransferMode::Binary, bool append = false);

  /**
   * @brief Send a command to the FTP server.
   *
   * While the most common FTP commands are provided as member
   * functions in the Ftp class, this method can be used
   * to send any FTP command to the server. If the command
   * requires one or more parameters, they can be specified
   * in @a parameter. If the server returns information, you
   * can extract it from the response using Response::getMessage().
   *
   * @param command Command to send.
   * @param parameter Command parameter.
   * @return Server response to the request.
   */
  [[nodiscard]] Response sendCommand(const std::string& command, const std::string& parameter = "");

 private:
  /**
   * @brief Receive a response from the server.
   *
   * This function must be called after each call to
   * sendCommand that expects a response.
   *
   * @return Server response to the request.
   */
  [[nodiscard]] Response getResponse();

  /**
   * @brief Utility class for exchanging data with the server on the data channel.
   */
  class DataChannel;

  TcpSocket m_commandSocket;    ///< Socket holding the control connection with the server.
  std::string m_receiveBuffer;  ///< Received command data that is yet to be processed.
};

}  // namespace sockpp
