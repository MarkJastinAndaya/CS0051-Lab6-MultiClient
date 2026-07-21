import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Scanner;

public class Client {

    // Server connection details
    private static final String SERVER_ADDRESS = "127.0.0.1";
    private static final int SERVER_PORT = 8080;

    public static void main(String[] args) {

        // Declare resources here so they are visible in the finally block
        Socket socket = null;
        PrintWriter out = null;
        BufferedReader in = null;
        Scanner scanner = null;

        try {
            scanner = new Scanner(System.in);

            socket = new Socket(SERVER_ADDRESS, SERVER_PORT);

            out = new PrintWriter(socket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            System.out.println("========================================");
            System.out.println("        JAVA TCP CLIENT");
            System.out.println("========================================");
            System.out.println();
            System.out.println("Connected to : " + SERVER_ADDRESS + ":" + SERVER_PORT);
            System.out.println();

            System.out.print("First Number : ");
            int firstNumber = Integer.parseInt(scanner.nextLine().trim());

            System.out.print("Second Number: ");
            int secondNumber = Integer.parseInt(scanner.nextLine().trim());

            String message = firstNumber + " " + secondNumber;

            System.out.println();
            System.out.println("Sending      : " + message);

            out.println(message);

            String response = in.readLine();

            System.out.println();
            System.out.println("Server Result: " + response);
            System.out.println();
            System.out.println("Connection closed.");

            System.out.println("========================================");
        }
        
        catch (UnknownHostException e) {
            System.out.println("\nERROR: Unable to resolve server address.");
        }

        catch (NumberFormatException e) {
            System.out.println("\nERROR: Please enter valid integers only.");
        }

        catch (IOException e) {
            System.out.println("\nERROR: Unable to communicate with the server.");
        }
        
        finally {
            // Close all resources safely, checking for null first
            try {
                if (in != null) {
                    in.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            if (out != null) {
                out.close();
            }

            try {
                if (socket != null) {
                    socket.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            if (scanner != null) {
                scanner.close();
            }
        }
    }
}
