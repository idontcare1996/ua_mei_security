import com.google.gson.JsonObject;

import java.io.*;
import java.lang.ClassNotFoundException;
import java.lang.Runnable;
import java.lang.Thread;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.stream.Collectors;

public class AuctionRepository {
    private ServerSocket server;
    private static final int PORT = 7777;

    private AuctionRepository() {
        try {
            server = new ServerSocket(PORT);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {
        AuctionRepository example = new AuctionRepository();
        example.handleConnection();
    }

    private void handleConnection() {


        while (true) {
            try {
                Socket socket = server.accept();
                new ConnectionHandler(socket);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
    class ConnectionHandler implements Runnable {
        private Socket socket;

        ConnectionHandler(Socket socket) {
            this.socket = socket;

            Thread t = new Thread(this);
            t.start();
        }

        public void run() {
            Auction auction = new Auction();
            try {

                ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());

                System.out.println((String) ois.readObject());

                ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
                oos.writeObject("Hi...");

                ois.close();
                 oos.close();
                socket.close();

                System.out.println("Waiting for client message...");
            } catch (IOException /*| ClassNotFoundException*/ e) {
                e.printStackTrace();
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
            }
        }
    }
}
