public class Message {
    private String type;
    private String action;
    private String data;

    public Message(){

    }
    public Message(String type, String action, String data)
    {
        this.type = type;
        this.action = action;
        this.data = data;
    }
    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getAction() { return action; }

    public void setAction(String action) { this.action = action;}

    public String getData() {
        return data;
    }

    public void setData(String data) {
        this.data = data;
    }
}
