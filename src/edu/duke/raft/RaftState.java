package edu.duke.raft;

import java.net.MalformedURLException;
import java.rmi.Naming;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.util.Timer;
import java.util.TimerTask;

public abstract class RaftState {
  // latest term the server has seen
  protected static int mCurrentTerm;
  // candidate voted for in current term; 0, if none
  protected static int mVotedFor;
  // log containing server's entries
  protected static RaftLog mLog;
  // index of highest entry known to be committed
  protected static int mCommitIndex;
  // index of highest entry applied to state machine
  protected static int mLastApplied;
  // lock protecting access to RaftResponses
  protected static Object mLock;
  // port for rmiregistry on localhost
  protected static int mRmiPort;


  // initializes the server's state
  public static void initializeServer (int currentTerm, 
				       int votedFor,
				       RaftLog log,
				       int commitIndex,
				       int lastApplied, 
				       int rmiPort) {
    mCurrentTerm = currentTerm;
    mVotedFor = votedFor;
    mLog = log;
    mCommitIndex = commitIndex;    
    mLastApplied = lastApplied;
    mLock = new Object ();
    mRmiPort = rmiPort;    
  } 

  // @param milliseconds for the timer to wait
  // @param a way to identify the timer when handleTimeout is called
  // after the timeout period
  // @return Timer object that will schedule a call to the state's
  // handleTimeout method. If an event occurs before the timeout
  // period, then the state should call the Timer's cancel method.
  protected final Timer scheduleTimer (long millis,
				       int timerID) {
    Timer timer = new Timer (false);
    TimerTask task = new TimerTask () {
	public void run () {
	  RaftState.this.handleTimeout (timerID);
	}
      };    
    timer.schedule (task, millis);
    return timer;
  }

  private final String getRmiUrl (int serverID) {
    return "rmi://localhost:" + mRmiPort + "/S" + serverID;
  }
  
  // called to make request vote RPC on another server
  // results will be stored in RaftResponses
  protected final void remoteRequestVote (int serverID,
					  int candidateTerm,
					  int candidateID,
					  int lastLogIndex,
					  int lastLogTerm) {
    new Thread () {
      public void run () {	
	String url = getRmiUrl (serverID);
	try {
	  RaftServer server = (RaftServer) Naming.lookup(url);
	  int response = server.requestVote (candidateTerm,
					     candidateID,
					     lastLogIndex,
					     lastLogTerm);
	  synchronized (RaftState.mLock) {
	    RaftResponses.setVote (serverID, 
				   response, 
				   candidateTerm);
	  }
	} catch (MalformedURLException me) {
	  System.out.println (me.getMessage());
	} catch (RemoteException re) {
	  System.out.println (re.getMessage());
	} catch (NotBoundException nbe) {
	  System.out.println (nbe.getMessage());
	}
      }
    }.start ();
  }  

  // called to make request vote RPC on another server
  protected final void remoteAppendEntries (int serverID,
					    int leaderTerm,
					    int leaderID,
					    int prevLogIndex,
					    int prevLogTerm,
					    Entry[] entries,
					    int leaderCommit) {
    new Thread () {
      public void run () {	
	String url = getRmiUrl (serverID);
	try {
	  RaftServer server = (RaftServer) Naming.lookup(url);
	  int response = server.appendEntries (leaderTerm,
					       leaderID,
					       prevLogIndex,
					       prevLogTerm,
					       entries,
					       leaderCommit);
	  synchronized (RaftState.mLock) {
	    RaftResponses.setAppendResponse (serverID, 
					     response, 
					     leaderTerm);
	  }
	} catch (MalformedURLException me) {
	  System.out.println (me.getMessage());
	} catch (RemoteException re) {
	  System.out.println (re.getMessage());
	} catch (NotBoundException nbe) {
	  System.out.println (nbe.getMessage());
	}
      }
    }.start ();
  }  

  // called to activate the state
  abstract public void go ();

  // @param candidate’s term
  // @param candidate requesting vote
  // @param index of candidate’s last log entry
  // @param term of candidate’s last log entry
  // @return 0, if server votes for candidate; otherwise, server's
  // current term
  abstract public int requestVote (int candidateTerm,
				   int candidateID,
				   int lastLogIndex,
				   int lastLogTerm);

  // @param leader’s term
  // @param current leader
  // @param index of log entry before entries to append
  // @param term of log entry before entries to append
  // @param entries to append (in order of 0 to append.length-1)
  // @param index of highest committed entry
  // @return 0, if server appended entries; otherwise, server's
  // current term
  abstract public int appendEntries (int leaderTerm,
				     int leaderID,
				     int prevLogIndex,
				     int prevLogTerm,
				     Entry[] entries,
				     int leaderCommit);

  // @param id of the timer that timed out
  abstract public void handleTimeout (int timerID);
}

  
