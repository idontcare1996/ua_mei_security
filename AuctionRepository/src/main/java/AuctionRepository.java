import java.io.IOException;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.lang.ClassNotFoundException;
import java.lang.Runnable;
import java.lang.Thread;
import java.net.ServerSocket;
import java.net.Socket;

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
        System.out.println("Waiting for client message...");

        // The server do a loop here to accept all connection initiated by the
        // client application.
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
                // Read a message sent by client application
                //ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
                InputStreamReader is = new InputStreamReader(socket.getInputStream());
                //auction.setData(((Auction) ois.readObject()).getData());
                String message = "";
                System.out.println(is);
                System.out.println("Message Received: " + is.read());

                // Send a response information to the client application
                ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
                oos.writeObject("Hi...");

                //ois.close();
                is.close();
                oos.close();
                socket.close();

                System.out.println("Waiting for client message...");
            } catch (IOException /*| ClassNotFoundException*/ e) {
                e.printStackTrace();
            }
        }
    }
}
